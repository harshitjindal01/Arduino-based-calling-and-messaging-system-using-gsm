#include <SoftwareSerial.h>
#include<String.h>
#include <LiquidCrystal.h>

SoftwareSerial mySerial(2,3);                            // These pins are connected to GSM module(  RX, TX )

LiquidCrystal lcd(14, 15, 16, 17, 18, 19);              // These are connected to LCD pins (RS, EN, D4 ,D5, D6, D7 ) respectively, Vdd-5V, Vss & R/W -GND

                                                                             
 String number= "";                                               

 String action= "WT";                  //String codes: RC =Receive call, RM= Receive msg, SC= Send calll, SM= Send message, WT= Wait
 
 // Receive sms Strings
 String Response ="";
 String sms="";
 String Type;
 String Caller_id;
 String Text;
 String SP_name="";                    
 
 char character;
 char quote= 0x22;


  // Global Flags 
  bool Send_m=false;
  bool sms_Receive_mode_off=true;
  bool Receive_mode=false;
  bool msg_Receive=false;
  bool time_registered=false;
  bool msg_fetched=false;
  bool on_call=false;
  bool start_Receive=false;
  bool flag=true;
  
  int sec,minutes;    // Clock variables
  long c_start;
  long c_time;
  
  int i=0;

  int indexOfQuotes[10];


  double time_start;
  double time_current;
  double operational_time;



 /********* Keypad Variables**********/
  int r1=11;
  int r2=10;
  int r3=9;
  int r4=8; 
  int c1=7;
  int c2=6;
  int c3=5;
  int c4=4;
  int colm1;
  int colm2;
  int colm3;
  int colm4;
  char value;
  //
  char num1;


void setup() 
  {
    Serial.begin(9600);
    Serial.println("GSM startrd");
    mySerial.begin(9600);
    mySerial.setTimeout(2000);
    Serial.setTimeout(2000);  
     initilise();
     lcd.begin(16, 2);
     get_SP();          
  }
  
  
void loop() 
           { 
               Serial.println("Action: "+ action);                       //Reports it current mode of working
               
               while(action=="WT")                                       // Its wait for SMS and Calls in this loop
              {
                if(sms_Receive_mode_off)                                 //So, This turns on the SMS recieve mode
                    { delay(1000);
                      On_sms_Receive_mode();
                    }
                
               if(flag)
                {
                  Serial.println("Receive_ready");
                  flag=false;
                  print_head("Connected to:");                           // Service provide name is printed on LCd
                  print_content(SP_name);
                  clear_Serial();                                      
                }
                   
        
                if(Receiving_on())                                        // FINALLY, the module is set to receive, Receive_on will beocome true in case msg or call arrives
                {
                  Extract_type();                                    
                }
                 
                else
                                {                                         // In case of no reciving, update the current signal strength
                                  update_signal_strength();
                                  get_request();                          //  Or, check if user pressed any button for callling or SMS
                                }
              }
              
              while(action=="SM")                                         // Sending Msg action
             {
                Serial.println("Enter number to message");
                print_head("Send SMS to");
                number= Take_input();                                      // Take input through swith matrix
                //LCD print for Send message
                bool success = send_sms(number);                   
                if(success)                                                // If sucessful go to wait state otherwise send again
                   { 
                     action="WT";                               
                   }
                   flag=true;
             }

              while(action=="SC")                                          //Sending call action, similar process as above
              {
                print_head("Enter Call num");
                Serial.println("Enter number to call");
                
                number = Take_input();
      

                if (valid_number())                                      // Check number is 10 digit long
                  {
                    print_head("Calling");
                    print_content(number);
                      delay(1000);
                      
                       send_call(number);  
                      
                      print_head("On line with");
                      print_content(number);
                      delay(1000);
                      clear_Serial();
                     if(on_call)
                      {terminate_call();}                                 // Waits here till the user is on call
                  }
                 
                 action="WT";
                 flag=true;
                }

              while(action=="RC")                                         // Recive call action
              {
                Serial.println("Press * to pick up or # to terminate");
                print_head("Call from");
                print_content(Caller_id);
                clear_Serial();
                WaitForPickup();
                //incall 
                if(on_call)                                                // Waits here till the user is on call
                  {terminate_call();}                               
                 Serial.println("Call response Recieved");
                 action="WT";
                 flag=true;
              }

              while(action=="RM")                                          // Recieve SMS action
              {
                Show_sms();
                action="WT";
                flag=true;
              }
           }

/*
 * Function to get the service provider(SP) name
 * Sets a Global varible: SP_name
*/

void get_SP (void)
 {  bool got_it=false;
     delay(1000);
     mySerial.println("AT");
     delay(500); 
     print_head("Connecting GSM");
    while(!( SP_name.indexOf('"')>0))
    { if(GSM_operational())
      {   
         mySerial.println("AT+COPS?");     //AT command for getting serivce provider name
         mySerial.println();         
      }
      
      delay(1000);

     while(mySerial.available())
                           {
                            char character=mySerial.read();
                            SP_name.concat(character);
                           }

    }
      // Extracton process
      SP_name= (SP_name.substring(SP_name.indexOf('"')+1,SP_name.lastIndexOf('"')));
      Serial.println("Connected to: "+ SP_name);
      
 }



// Fuciton to print current signal strength on lcd

void update_signal_strength (void)
  {     String Network;
         long Strength;   
         mySerial.println("AT+CSQ");  
         mySerial.println();         
      
      delay(500);
      while(mySerial.available())
        {
                 char character=mySerial.read();
                 Network.concat(character);
        }

        Network=Network.substring(Network.indexOf(':')+2,Network.indexOf(','));
        Strength= Network.toInt();                           // Strength Int value here
       
        Strength=(Strength*100/31);                          // MAX strength= 31
        lcd.setCursor(13,2);
        lcd.print(int(Strength));
        lcd.print('%');
  }
 
 //It recives a the char value of key pressed and stores it into 
void get_request (void)
 {
   value=Return_Keypad_Values();
   event(value);
 }

// Select the apt mode as per the input
void event(char func)
{
  switch (func)
  {

    case 'A':
       action="SC";                   //Send call
       break;
    case 'B':
       action="SM";                  // Send Message
       break;
    case 'C':                       
      action="RC";                   // Receive Call
      break;
    default:
      action="WT";                   // Wait for response
     break;
 }
}



/*
 * Input: (string:num,) 
 * Output bool( t=sent f=unsent)
 * Function to send sms to number
*/
bool send_sms (String number)
 {
   delay(1000);
   mySerial.println("AT");
   delay(500);
   if(GSM_operational())
     {
       mySerial.println("AT+CMGF=1");
       delay(500);
     }
   
   if(GSM_operational())
     {
       mySerial.print("AT+CMGS=\"");                           // Send the SMS number
       mySerial.print(number);
       mySerial.println("\"");
        
        delay(1000);
       mySerial.print("GSM bot functonal");                     // SMS BODY here in case u want change
       // mySerial.print(i); 
       delay(500);
       
       mySerial.write(0x1A);
       mySerial.write(0x0D);
       mySerial.write(0x0A); 
       Serial.println("SMS sent");
       
       print_head("SMS Sent  to");
       print_content(number);
       delay(2000);
       
       return(true); //SMS sent succussfuly 
     } 
    return(false);   // Failed attempt
 }

/*
 * Input: (string:num,) 
 * Output bool( t=sent f=failed)
 * Function to send call to number
*/
 bool send_call (String number)
  {
     
     mySerial.println("AT");
     delay(500); 

     if(GSM_operational())
      {                  
        //Number dialing
         Serial.println("Calling to :" +number);
         print_head("Calling to");
         print_content(number);
        mySerial.println("ATD"+ number +";");                // AT command for dialing up the number
        mySerial.println();
        on_call=true;
        return(true);
      } 
      return(false);
  }


// This is to switch on the messaging mode of Gsm
void On_sms_Receive_mode (void)
 {
      mySerial.print("ATE0");       
       delay(500);
         
         if(GSM_operational())
            mySerial.print("AT");
          delay(500);  
       
         if(GSM_operational())
            mySerial.print("AT+CMGF=1");                               // Setup in msging mode
         delay(500);
    
         if (GSM_operational())
           {
             mySerial.print("AT+CNMI=2,2,0,0,0\r" );                   //Prints the msg on serial as soon as it arrives
             delay(500);
             
             while(mySerial.available())
              {
                char response = mySerial.read();
                Serial.println(response);
              }

               Serial.println("Receive mode On");
                sms_Receive_mode_off=false;  //turn it on off
           }
 }


 /*
  * Input: none
  * Output: True: A response( call or sms) incoming, Or false
  *
 */
 bool Receiving_on (void)
   { 
     bool  Response_available=false;
      
     if(mySerial.available())                          
                    {
                      while(!halt_fetch())                          //In case of incoming recieve until halt_fetch() gives true   
                      { 
                        while(mySerial.available())
                           {
                            if(!time_registered)                      //Capture the time of start of message receiving
                                {
                                  time_start=millis(); 
                                  time_registered=true; 
                                }
                            char character=mySerial.read();
                            Response.concat(character);
                            Serial.print(character);                   // Store as a string
                            }
                     
                      } 
                        
                          Serial.println("Response Received");         //Looks like we got something
                          Response_available=true;
                          msg_fetched=false;
                          flag=true;
    
                    }
       return (Response_available);
   }

 /*
 *The function is created to halt or to indicate the end of receiving
 *It does that by a timeout of 3sec or Response Text limit of 500 characters
 *Input: none
 *Output: Boolean, T= halt fetching F= Wait for message 

*/
bool halt_fetch (void)
  { 
    bool halt=false;

    if(time_registered)
    {
      time_current=millis();
      operational_time=time_current-time_start;
    }

    if(operational_time>3000 || Response.length()==200 )                        // Halt condition
     { 
       halt=true; 
       operational_time=0;
     }
   return halt;
  }


 /*
 * It extracts the Response and caller id
 * It does that by quotes position.
 * Caller id is between first and second quotes
 * While, Text message is after last quotes
*/

 void Extract_type (void)
  { 
                    if(valid_input())
                     {
                      Serial.println("Valid respone");
                      extract();
                     
                      
                  Serial.println(Response);                           //In case u want to see everything incoming
                  Serial.println("Type: ");
                  Serial.print(Type);
                  Serial.println("Caller id : ");
                  Serial.println(Caller_id);
                  Serial.println("Text: ");
                  Serial.println(Text);
                  callORsms();
                  Serial.print(Caller_id);
                     }

                    time_registered=false;
                  
                   Response="";                                           //Clear string for refresh incoming
    
  }

  
  /*
   * Checks the validity condition, 
   * True: Its call or msg Resonse
   * False: it is some junk
  */
  bool valid_input (void)
  {
    bool validity;
    
    validity=(( Response.indexOf('+') > 0) && (Response.indexOf('"')>0 ));   //If the reponse has these two thing that means it is a 'real' response
    
    if(!validity)
    {
      Serial.println("invalid input");
      
    }
    
    return (validity);
     
  }
  
  // Find the indexes of all the quotes in the stirng and sets them up in gloablevariable: indexOfQuotes[index]
  void extract(void)
 { 
    int Length,i,index=0;
    
    Length=Response.length();
    for(i=0;i<=Length;i++)
    {
          if(Response.charAt(i)==quote)
          {
            indexOfQuotes[index]=i;
            index++;
          }
    }

    Type=Response.substring(1,indexOfQuotes[0]);
    Caller_id=Response.substring(indexOfQuotes[0]+1,indexOfQuotes[1]);
    Text=Response.substring(indexOfQuotes[5]+3);
   Serial.println("Extracted");
 }


 // Determine weather the response is of call or sms
 void callORsms (void)
 { 
   if( Type.indexOf("LIP")>0)                      //Call string consist this( +CLIP)
      { action="RC";
        Serial.println("Call from: ");}
    else if(Type.indexOf("MT")>0 )                 // Msg stirng consist (+CMT)
        { action="RM";
          Serial.println("Message from: ");}
 }



 // Waits till customer press * or # 
 void WaitForPickup (void)
  {
    char key;
    bool user_wait = true;  //default state
      while(user_wait)
      { user_wait=check_termination();
        
        key=Return_Keypad_Values();
        if(key=='*')                          //picking up reponse
        {
          mySerial.println("ATA");
            mySerial.println();
          Serial.println("Call picked up");
          print_head("Call picked up");
          print_content(Caller_id);
          delay(1000);
          user_wait=false;
          on_call=true;
        }
        
        if(key=='#')                              //Termination action
        {
          mySerial.println("ATH");   
          mySerial.println();
            Serial.println("Call Terminated");
            print_head("Call Terminated");
            delay(1000);
          print_content(Caller_id);
          on_call=false;
            user_wait=false;
        }
        
      }
   
  }


 /*
  * This function is used after two user get connected on a call
  * It waits '#' to terminate or 'NO CARRIER' on serial monitor
  * It updates clock untill waiting
  * */

  void terminate_call (void)
  {
    char key;
    bool user_wait = true;  //default state
    start_clock();
    while(user_wait)
      { 
        
        user_wait=check_termination();

        key=Return_Keypad_Values(); 
        
        if(key=='#')
        {
            mySerial.println("ATH");   //Termination action
            mySerial.println();
            Serial.println("Call Terminated");
            print_head("Call Terminated");
            delay(1000);
            print_content(Caller_id);
            user_wait=false;
        }

        else
        {
          update_clock();
        }

      }
    on_call=false;
  }

  // Function to start a clock
  void start_clock (void)
    {
       lcd.clear();
      c_start=millis();
      sec=0;
      minutes=0;
      lcd.print("On call");
    }

  // Function to update the value as arduino internal clock
  
    void update_clock (void)
    {
       long current= millis();


       if(current-c_start>1000)
       {
          sec++;
          c_start=current;
       }

       if(sec>59)
       {
         minutes++;
         sec=-0;
       }
       
       lcd.setCursor(0,1);
              
               if(minutes<10)
                 {lcd.print('0');}
                  lcd.print(minutes);
                  lcd.print(':');
               if(sec<10)
                 {lcd.print('0');}
                  lcd.print(sec);
    }

  // Fuction to Show sms on a LCD
 void Show_sms (void)
  {
   print_head("SMS from");
   print_content(Caller_id);
    char key;
   
   
   // Enhance modularity
   bool user_wait = true;
         while(user_wait)
         {key=Return_Keypad_Values();
           if(key=='*') 
           {
             print_head(Text.substring(0,16));                                // This can scroll SMS 
             print_content(Text.substring(16,32));
             delay(2000);
             print_head(Text.substring(16,32));
              print_content(Text.substring(32,48));
             delay(2000);      //A scroll fuction can be made 
             Serial.println(Text);
             user_wait=false;
           }
           
           if(key=='#')
           {
               print_head("OK");
               Serial.println("MSG Terminated");
               delay(500);
               user_wait=false;
           }
           
         }
  }

 
 //True if starkey is pressed
 bool Starkey_pressed (void)
  { char key;
   key=Return_Keypad_Values();
   return (key=='*');
  }

 //True if Hashkey is pressed
 bool Hashkey_pressed (void)
 { char key;
    key=Return_Keypad_Values();
   return (key=='#');
 }

 //Check if 'NO CARRIER' is printer on Serial monitor

 bool check_termination (void)
  {
    bool check=true;
    String listen_no="";

    while(mySerial.available())
      {
        char data= mySerial.read();
        Serial.print(data);
        listen_no.concat(data);
      }
      
    if(listen_no.indexOf("CAR")>0)   // I check for only CAR
     { check=false; }
     
     return check;
  }
 

// A Fuciton to check the lenth of number calling should be 10 + ('+91' country code) =13
 bool valid_number (void)
 {
   bool valid=false;
   if(number.length()==13)     // condition here
   {valid=true;}
   else
   {print_head("Invalid input");
    delay(1000);
   }
   return valid;
 }

 //Essential command to determine the state of GSM module
 bool GSM_operational(void)
  {
    int count =0;
    bool Status=false;
    mySerial.println();
    while(1)
    { 
      if (mySerial.available()>0)
       { 
        char data = mySerial.read();
        if( data == 'O' || data == 'K')         //Its working properly
          {
            Serial.println("OK");
            Status=true;
            break;
            }
        if( data == 'E' || data == 'R' || data== 'O')  // Working yet busy with some thing else
          {
            Serial.println("GSM not functional");
             Status=false;
             break;
          }   
       }
      count++;
      delay(10);
      if (count == 100)
      {
        Serial.println("GSM not connected");            // No reponse for AT commands
       Status=false;
       break;
      }
     }
      return Status;
    }

    void clear_Serial (void)
    {
      while(mySerial.available())
       {
        char  character=mySerial.read();
        Serial.print(character);
       }
      
    }

/*************************************************************
 *                      Keypad Firmware Ahead                *
 *************************************************************/

/*
 * input: none
 * Output:  A 13 digit number
 * Waits till user enter  a ten Digit number
*/
 String Take_input (void)
{ String num="+91";
  int len=0;
  int len2;
  while (len <= 13)
  {
   len=num.length();
   num1=Return_Keypad_Values();
   if ((num1!='A')&&(num1!='B')&&(num1!='C')&&(num1!='a'))
    {
      if ((num1!='#') && (num1!='*') && (num1!='D'))
         {num+=String(num1);
         print_content(num);
         Serial.println(num);
         }
      
      else if (num1=='*')
         {
           num.setCharAt(len-1,'*');
           print_content(num);
           num.remove(len-1); 
          
         }
      else if (num1=='#')
        { 
          Serial.println(num);
          break;  
        }
     else if(num1=='D')
     {
      break;
     }
    }
    
  }
  return num;
}


 
void initilise()
{
  pinMode(r1,OUTPUT);
  pinMode(r2,OUTPUT);
  pinMode(r3,OUTPUT); //use in setup
  pinMode(r4,OUTPUT);
  pinMode(c1,INPUT);
  pinMode(c2,INPUT);
  pinMode(c3,INPUT);
  pinMode(c4,INPUT);
  Serial.begin(9600);
  digitalWrite(c1,HIGH);
  digitalWrite(c2,HIGH);
  digitalWrite(c3,HIGH);
  digitalWrite(c4,HIGH);
}
void row1()
{
  digitalWrite(r1,LOW);
  digitalWrite(r2,HIGH);
  digitalWrite(r3,HIGH);
  digitalWrite(r4,HIGH);
}
void row2()
{
  digitalWrite(r1,HIGH);
  digitalWrite(r2,LOW);
  digitalWrite(r3,HIGH);
  digitalWrite(r4,HIGH);
}
void row3()
{
  digitalWrite(r1,HIGH);
  digitalWrite(r2,HIGH);
  digitalWrite(r3,LOW);
  digitalWrite(r4,HIGH);
}
void row4()
{
  digitalWrite(r1,HIGH);
  digitalWrite(r2,HIGH);
  digitalWrite(r3,HIGH);
  digitalWrite(r4,LOW);
}
void ReadRows()
{ 
  colm1=digitalRead(c1);
  colm2=digitalRead(c2);
  colm3=digitalRead(c3);
  colm4=digitalRead(c4);
}
char Return_Keypad_Values(void)
{
  row1();
  ReadRows();
  delay(100);
  if(colm1==LOW)
         {
           Serial.println("1");
           delay(200);
           return '1';
         }
  else if(colm2==LOW)
         {
           Serial.println("2");
           delay(200);
           return '2';
         }
   else if(colm3==LOW)
         {
           Serial.println("3");
           delay(200);
           return '3';
         }
   else if(colm4==LOW)
        {
          Serial.println("A");
          delay(200);
          return 'A';
        }
        
   row2();
   ReadRows();
   delay(100);
   if(colm1==LOW)
         {
           Serial.println("4");
           delay(200);
           return '4';
         }
   else if(colm2==LOW)
         {
           Serial.println("5");
           delay(200);
           return '5';
         }
   else if(colm3==LOW)
         {
           Serial.println("6");
           delay(200);
           return '6';
         }
   else if(colm4==LOW)
        {
          Serial.println("B");
          delay(200);
          return 'B';
        }
   row3();
   ReadRows();
   delay(100);
   if(colm1==LOW)
         {
           Serial.println("7");
           delay(200);
           return '7';
         }
   else if(colm2==LOW)
         {
           Serial.println("8");
           delay(200);
           return '8';
         }
   else if(colm3==LOW)
         {
           Serial.println("9");
           delay(200);
           return '9';
         }
   else if(colm4==LOW)
        {
          Serial.println("C");
          delay(200);
          return 'C';
        }
   row4();
   ReadRows();
   delay(100);
   if(colm1==LOW)
         {
           Serial.println("*");
           delay(200);
           return '*';
         }
   else if(colm2==LOW)
         {
           Serial.println("0");
           delay(200);
           return '0';
         }
   else if(colm3==LOW)
         {
           Serial.println("#");
           delay(200);
           return '#';
         }
   else if(colm4==LOW)
        {
          Serial.println("D");
          delay(200);
          return 'D';
        } 
 return 'a';       
}

/*************************************************************
 *                     LCD functions Ahead             *
 *************************************************************/

 //Print out the Heading On lCD 
 void print_head (String str)
  { lcd.clear();
     lcd.setCursor(0,0);
     lcd.print(str);
    }
    
 //Print secondary content on LCD
 void print_content (String str)
  {
   lcd.setCursor(0,1);
   lcd.print(str);
  }
