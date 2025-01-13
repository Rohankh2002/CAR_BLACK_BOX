/*
 * Name: Rohan K H
 * Date: 05/01/2025
 * Description: The "Car Black Box" project uses Embedded C programming to monitor and record the time,
 *              gear position, and speed of a car during the last 10 events. An external EEPROM is
 *              utilized to store this data, ensuring retention even when the system powers off. 
 *              The project integrates a CLCD (Character LCD) for real-time display of information 
 *              and employs an ADC (Analog-to-Digital Converter) to measure and process the car's speed. 
 *              This system is implemented on a development kit for prototyping and testing.
 */
#include "main.h"
#include "clcd.h"
#include "mkp.h"
#include "adc.h"
#include "i2c.h"
#include "external_eeprom.h"
#include "uart.h"
#include "ds1307.h"

unsigned char clock_reg[3];    // Array to store time data
unsigned char time[9];         // Array to store time in the format HH:MM:SS

void display_time(void);  // Function to display the time on the CLCD
static void get_time(void);  // Function to get the current time from DS1307 RTC
void download_data(char view_data[][11],int total_event);  // Function to download data from EEPROM
void print_time(int hour,int minute,int second);  // Function to print the time
void blink_hour(int minute,int second);  // Function to blink the hour section of the time
void blink_minute(int hour,int second);  // Function to blink the minute section of the time
void blink_second(int hour,int minute);  // Function to blink the second section of the time

int  adc_reg_val,ev,ev1,ev2;   // Variables for ADC values and event data
int i = 0 ,time_mode = 0,scroll = -1, m = 0, event_count = 0, total_event = 0, delay=0, event_flag = 0, log = 0, address = 0;  // Various flags and counters
int second = 12,minute = 12 ,hour= 12;   // Default time set to 12:12:12
char view_data[11][11] = {};  // Array to store the viewable data from EEPROM
unsigned char key = 0, pre_key = 0;  // Key input and previous key state
int wait = 0;  // Delay counter
char eeprom_data[10];  // Buffer for storing data to be written to EEPROM
char eeprom_read[10];  // Buffer for storing data read from EEPROM
char GEAR[8][3] = {"ON","GN","GR","G1","G2","G3","G4","C "};  // Array for gear data (statuses)
char LOG[5][13] = {"view log    ","clear log   ","download log","set time    "};  // Menu options for logs
void view_stored_dat(int m, char view_data[][11]);  // Function to view stored data from EEPROM

// Initialize all necessary peripherals and configurations
void init_config(void)
{
    init_clcd();  // Initialize the CLCD
    init_mkp();   // Initialize the matrix keypad
    init_adc();   // Initialize the ADC
    init_i2c();   // Initialize the I2C communication
    init_uart();  // Initialize the UART communication
    init_ds1307();  // Initialize the DS1307 RTC
}

void main(void)
{
    init_config();  // Initialize all configurations
    
    while(1)
    {
        key = read_matrix_keypad(STATE);  // Read key input from the keypad
        
        if(log == 0)  // If not in log mode
        {
            get_time();  // Get the current time from RTC
            display_time();  // Display the time on the CLCD

            if(key == SW4)  // If SW4 is pressed, enter log mode
            {
                log = 1;
            }
            
            clcd_print("  TIME    EV  SP", LINE1(0));  // Print the time label on the CLCD
            
            adc_reg_val = read_adc(CHANNEL4);  // Read ADC value from channel 4
            ev = (adc_reg_val / 10.33);  // Calculate the event value based on ADC data
            ev1 = (ev / 10) + 48;  // Convert the event value to ASCII characters
            ev2 = (ev % 10) + 48;  // Convert the event value to ASCII characters

            clcd_putch(ev1, LINE2(14));  // Display the first part of the event value
            clcd_putch(ev2, LINE2(15));  // Display the second part of the event value

            // If SW2 is pressed and event count is less than 6, store event data in EEPROM
            if(key == SW2 && i < 6)
            {  
                if(event_count > 9)
                    event_count = 0;  // Reset event count if it exceeds 9

                i++;  // Increment the gear index
             
                eeprom_data[0] = time[0];  // Store current time data
                eeprom_data[1] = time[1];
                eeprom_data[2] = time[3];
                eeprom_data[3] = time[4];
                eeprom_data[4] = time[6];
                eeprom_data[5] = time[7];
                eeprom_data[6] = GEAR[i][0];  // Store current gear state
                eeprom_data[7] = GEAR[i][1]; 
                eeprom_data[8] = ev1;  // Store event value part 1
                eeprom_data[9] = ev2;  // Store event value part 2

                int k = 0;
                // Write event data to EEPROM
                for(int i = (event_count * 10); i < ((10 * event_count) + 10); i++)
                {
                    write_external_eeprom(i, eeprom_data[k]);
                    k++;
                }
                
                event_count++;  // Increment event count
                total_event++;  // Increment total event count
            }
            // Similar logic for SW3 and SW1 to decrease or set gear index
            else if(key == SW3 && i > 1)
            {
                if(event_count > 9)
                    event_count = 0;  // Reset event count if it exceeds 9

                i--;  // Decrement the gear index
                // Similar EEPROM write logic as for SW2
                eeprom_data[0] = time[0];
                eeprom_data[1] = time[1];
                eeprom_data[2] = time[3];
                eeprom_data[3] = time[4];
                eeprom_data[4] = time[6];
                eeprom_data[5] = time[7];
                eeprom_data[6] = GEAR[i][0];
                eeprom_data[7] = GEAR[i][1];
                eeprom_data[8] = ev1;
                eeprom_data[9] = ev2;
                int k=0;
                for(int i=(event_count*10);i<((10*event_count)+10);i++)
                {
                    write_external_eeprom(i, eeprom_data[k]);
                    k++;
                }

                event_count++;
                total_event++;
            }
            else if(key == SW1)  // Reset gear to the last state when SW1 is pressed
            {
                if(event_count > 9)
                    event_count = 0;  // Reset event count if it exceeds 9
                i = 7;  // Set gear index to 7

                eeprom_data[0] = time[0];
                eeprom_data[1] = time[1];
                eeprom_data[2] = time[3];
                eeprom_data[3] = time[4];
                eeprom_data[4] = time[6];
                eeprom_data[5] = time[7];
                eeprom_data[6] = GEAR[i][0];
                eeprom_data[7] = GEAR[i][1];
                eeprom_data[8] = ev1;
                eeprom_data[9] = ev2;

                int k=0;
                // Write event data to EEPROM
                for(int i = (event_count*10); i < ((10*event_count) + 10); i++)
                {
                    write_external_eeprom(i, eeprom_data[k]);
                    k++;
                }

                event_count++;
                total_event++;
                i = 7;  // Reset gear index to 7
            }
            else if ((key == SW2 || key == SW3) && i == 7)  // If gear index is 7, set to 1
            {
                if(event_count > 9)
                    event_count = 0;  // Reset event count if it exceeds 9
                i = 1;  // Set gear index to 1
                
                // Store time and event data in EEPROM
                eeprom_data[0] = time[0];
                eeprom_data[1] = time[1];
                eeprom_data[2] = time[3];
                eeprom_data[3] = time[4];
                eeprom_data[4] = time[6];
                eeprom_data[5] = time[7];
                eeprom_data[6] = GEAR[i][0];
                eeprom_data[7] = GEAR[i][1];
                eeprom_data[8] = ev1;
                eeprom_data[9] = ev2;

                int k = 0;
                // Write event data to EEPROM
                for(int i = (event_count*10); i < ((10*event_count)+10); i++)
                {
                    write_external_eeprom(i, eeprom_data[k]);
                    k++;
                }

                event_count++;
                total_event++;
            }

            // Update the CLCD with current gear and event data
            clcd_print("  ", LINE2(8));
            clcd_print(GEAR[i], LINE2(10));
            clcd_print("  ", LINE2(12));
            scroll = -1;  // Reset scroll index
        }
        else if(log == 1)  // If in log mode
        {
            if(key == SW5)  // Exit log mode if SW5 is pressed
            {
                log = 0;
            }
            
            // Handle scrolling through log options
            if((key == SW2 && scroll < 3) || scroll == -1)
            {
               scroll++;  // Move to the next log option
            }
            
            if(key == SW1 && scroll > 0)
            {
               scroll--;  // Move to the previous log option
            }

            // Display the log menu options on the CLCD
            if(scroll == 0)
            {
                clcd_print("   *", LINE1(0));
                clcd_print(LOG[0], LINE1(4));
                clcd_print("    ", LINE2(0));
                clcd_print(LOG[1], LINE2(4));
            }
            else if(scroll == 1)
            {
                clcd_print("    ", LINE1(0));
                clcd_print(LOG[0], LINE1(4));
                clcd_print("   *", LINE2(0));
                clcd_print(LOG[1], LINE2(4));
            }
            else if(scroll == 2)
            {
                clcd_print("   ", LINE1(0));
                clcd_print(LOG[1], LINE1(4));
                clcd_print("   *", LINE2(0));
                clcd_print(LOG[2], LINE2(4));
            }
            else if(scroll == 3)
            {
                clcd_print("    ", LINE1(0));
                clcd_print(LOG[2], LINE1(4));
                clcd_print("   *", LINE2(0));
                clcd_print(LOG[3], LINE2(4));
            }

            if(key == SW4)
            {
                log = 2;  // Proceed to next log view mode
                scroll;
            }
        }
        else if(log == 2)  // If in second log view mode
        {
            if(key == SW5)  // Exit back to first log mode
            {
                log = 1;
            }
            
            // Handle viewing, clearing, and downloading logs
            if(scroll == 0)  // Viewing stored data
            {         
                if(total_event == 0)  // If no data in EEPROM
                {
                    clcd_print("EEPROM MEMORY   ", LINE1(0));
                    clcd_print("IS EMPTY        ", LINE2(0));
                }
                else if(total_event > 0 && total_event < 10)
                {
                    event_flag++;
                    for(int i = 0; i < 10; i++)  // Read event data from EEPROM
                    {
                        for(int j = 0; j < 10; j++)
                        {
                            view_data[i][j] = read_external_eeprom(i*10 + j);
                        }
                    }
                }
                else
                {
                    event_flag++;
                    int k = 0;
                    int count = 10;
                    i = ((total_event) % 10);
                    while(count)  // Loop to read more data if more than 10 events
                    {
                        int n = 0;
                        for(int j = i * 10; j < (i * 10) + 10; j++)
                        {
                            view_data[k][n] = read_external_eeprom(j);
                            n++;
                        }
                        k++;
                       
                        if(i > 9)
                            i = 0;
                        else
                            i++;
                            
                        count--;
                    }
                }

                if(event_flag)  // If event flag is set, display stored data
                {
                    view_stored_dat(m, view_data);
                }

                if(total_event < 10 && event_flag != 0)
                {
                    if((key == SW1 && m < total_event-1) && event_flag != 0)
                    {
                        m++;  // Move to next event
                    }
                    else if((key == SW2 && m > 0) && event_flag != 0)
                    {
                        m--;  // Move to previous event
                    }
                }
                else
                {
                    if((key == SW1 && m < 9) && event_flag != 0)
                    {
                        m++;  // Move to next event
                    }
                    else if((key == SW2 && m > 0) && event_flag != 0 )
                    {
                        m--;  // Move to previous event
                    }
                }
            }
            else if(scroll == 1)  // Clear EEPROM memory
            {
                total_event = 0;  // Reset event count
                event_flag = 0;  // Reset event flag
                clcd_print("EEPROM CLEARED  ", LINE1(0));
                clcd_print("SUCCESSFULLY    ", LINE2(0));
            }
            else if(scroll == 2)  // Download logs to terminal
            {
                if(total_event == 0)  // If no data in EEPROM
                {
                    puts("EEPROM MEMORY   ");
                    puts("\n\r");
                    puts("IS EMPTY        ");
                    puts("\n\r");
                }
                else if(total_event > 0 && total_event <= 10)
                {
                    for(int i = 0; i < total_event; i++)  // Read and send data to terminal
                    {
                        for(int j = 0; j < 10; j++)
                        {
                            view_data[i][j] = read_external_eeprom(i*10 + j);
                        }
                    }
                    download_data(view_data, total_event);
                }
                else
                {
                    int k = 0;
                    int count = 10;
                    i = ((total_event) % 10) + 1;
                    while(count)  // If more than 10 events, read in blocks
                    {
                        int n = 0;
                        for(int j = i * 10; j < (i * 10) + 10; j++)
                        {
                            view_data[k][n] = read_external_eeprom(j);
                            n++;
                        }
                        k++;
                    
                        if(i > 9)
                            i = 0;
                        else
                            i++;

                        count--;
                    }
                    view_data[11][11] = '\0';  // Null-terminate the data
                    download_data(view_data, total_event);  // Download the data
                }  
                log = 0;  // Exit log mode after download
            }               

            else if(scroll == 3)  // Setting time mode
            {
                clcd_print("  Setting time  ", LINE1(0));

                if(key == SW2)  // Change time mode if SW2 is pressed
                {
                    if(time_mode > 2)
                        time_mode = 1;
                    else
                        time_mode++;
                }

                if(time_mode == 0)  // Display current time and transition to next mode
                {
                    hour = (((time[0] - 48) * 10) + (time[1] - 48));
                    minute = (((time[3] - 48) * 10) + (time[4] - 48));
                    second = (((time[6] - 48) * 10) + (time[7] - 48));
                    time_mode++;
                }                   

                else if(time_mode == 1)  // Adjust the hour if in hour setting mode
                {
                    if(key == SW1)
                    {
                        if(hour <= 22)
                            hour++;
                        else
                            hour = 0;
                    }
                    if(wait++ < 250)
                        print_time(hour, minute, second);
                    else if(wait >= 250 && wait <= 500)
                        blink_hour(minute, second);
                    else
                        wait = 0;
                }

                else if(time_mode == 2)  // Adjust the minute if in minute setting mode
                {
                    if(key == SW1)
                    {
                        if(minute <= 58)
                            minute++;
                        else
                            minute = 0;
                    }
                    if(wait++ < 250)
                        print_time(hour, minute, second);
                    else if(wait >= 250 && wait <= 500)
                        blink_minute(hour, second);
                    else
                        wait = 0;
                }

                else if(time_mode == 3)  // Adjust the second if in second setting mode
                {
                    if(key == SW1)
                    {
                        if(second <= 58)
                            second++;
                        else
                            second = 0;
                    }
                    if(wait++ < 250)
                        print_time(hour, minute, second);
                    else if(wait >= 250 && wait <= 500)
                        blink_second(hour, minute);
                    else
                        wait = 0;
                }

                if(key == SW4)  // Save time and exit setting mode
                {
                    write_ds1307(HOUR_ADDR, (((hour / 10) << 4) | (hour % 10)));
                    write_ds1307(MIN_ADDR, (((minute / 10) << 4) | (minute % 10)));
                    write_ds1307(SEC_ADDR, (((second / 10) << 4) | (second % 10)));
                    log = 1;  // Return to log mode
                    time_mode = 0;  // Reset time mode
                }
            }
        }
    }
}

// Function to print the time in HH:MM:SS format
void print_time(int hour, int minute, int second)
{
    clcd_print("    ", LINE2(0));  // Clear the first part of the line
    clcd_putch((hour / 10) + '0', LINE2(4));  // Print the tens digit of the hour
    clcd_putch((hour % 10) + '0', LINE2(5));  // Print the ones digit of the hour
    clcd_putch(';', LINE2(6));  // Separator between hour and minute
    clcd_putch((minute / 10) + '0', LINE2(7));  // Print the tens digit of the minute
    clcd_putch((minute % 10) + '0', LINE2(8));  // Print the ones digit of the minute
    clcd_putch(';', LINE2(9));  // Separator between minute and second
    clcd_putch((second / 10) + '0', LINE2(10));  // Print the tens digit of the second
    clcd_putch((second % 10) + '0', LINE2(11));  // Print the ones digit of the second
    clcd_print("    ", LINE2(12));  // Clear the remaining part of the line
}

// Function to blink the hour part of the time display
void blink_hour(int minute, int second)
{
    clcd_print("    ", LINE2(0));  // Clear the first part of the line
    clcd_putch(' ', LINE2(4));  // Blank space for the hour tens digit
    clcd_putch(' ', LINE2(5));  // Blank space for the hour ones digit
    clcd_putch(';', LINE2(6));  // Separator between hour and minute
    clcd_putch((minute / 10) + '0', LINE2(7));  // Print the tens digit of the minute
    clcd_putch((minute % 10) + '0', LINE2(8));  // Print the ones digit of the minute
    clcd_putch(';', LINE2(9));  // Separator between minute and second
    clcd_putch((second / 10) + '0', LINE2(10));  // Print the tens digit of the second
    clcd_putch((second % 10) + '0', LINE2(11));  // Print the ones digit of the second
    clcd_print("    ", LINE2(12));  // Clear the remaining part of the line
}

// Function to blink the minute part of the time display
void blink_minute(int hour, int second)
{
    clcd_print("    ", LINE2(0));  // Clear the first part of the line
    clcd_putch((hour / 10) + '0', LINE2(4));  // Print the tens digit of the hour
    clcd_putch((hour % 10) + '0', LINE2(5));  // Print the ones digit of the hour
    clcd_putch(';', LINE2(6));  // Separator between hour and minute
    clcd_putch(' ', LINE2(7));  // Blank space for the minute tens digit
    clcd_putch(' ', LINE2(8));  // Blank space for the minute ones digit
    clcd_putch(';', LINE2(9));  // Separator between minute and second
    clcd_putch((second / 10) + '0', LINE2(10));  // Print the tens digit of the second
    clcd_putch((second % 10) + '0', LINE2(11));  // Print the ones digit of the second
    clcd_print("    ", LINE2(12));  // Clear the remaining part of the line
}

// Function to blink the second part of the time display
void blink_second(int hour, int minute)
{
    clcd_print("    ", LINE2(0));  // Clear the first part of the line
    clcd_putch((hour / 10) + '0', LINE2(4));  // Print the tens digit of the hour
    clcd_putch((hour % 10) + '0', LINE2(5));  // Print the ones digit of the hour
    clcd_putch(';', LINE2(6));  // Separator between hour and minute
    clcd_putch((minute / 10) + '0', LINE2(7));  // Print the tens digit of the minute
    clcd_putch((minute % 10) + '0', LINE2(8));  // Print the ones digit of the minute
    clcd_putch(';', LINE2(9));  // Separator between minute and second
    clcd_putch(' ', LINE2(10));  // Blank space for the second tens digit
    clcd_putch(' ', LINE2(11));  // Blank space for the second ones digit
    clcd_print("    ", LINE2(12));  // Clear the remaining part of the line
}

// Function to display stored data from the EEPROM
void view_stored_dat(int m, char view_data[][11])
{
    clcd_print("#   TIME   EV SP", LINE1(0));  // Header for the stored data
    clcd_putch(m + '0', LINE2(0));  // Print the index of the event (m)
    clcd_putch(' ', LINE2(1));  // Blank space
    clcd_putch(view_data[m][0], LINE2(2));  // Print the first digit of the hour
    clcd_putch(view_data[m][1], LINE2(3));  // Print the second digit of the hour
    clcd_putch(';', LINE2(4));  // Separator between hour and minute
    clcd_putch(view_data[m][2], LINE2(5));  // Print the first digit of the minute
    clcd_putch(view_data[m][3], LINE2(6));  // Print the second digit of the minute
    clcd_putch(';', LINE2(7));  // Separator between minute and second
    clcd_putch(view_data[m][4], LINE2(8));  // Print the first digit of the second
    clcd_putch(view_data[m][5], LINE2(9));  // Print the second digit of the second
    clcd_putch(' ', LINE2(10));  // Blank space
    clcd_putch(view_data[m][6], LINE2(11));  // Print the first character of the gear state
    clcd_putch(view_data[m][7], LINE2(12));  // Print the second character of the gear state
    clcd_putch(' ', LINE2(13));  // Blank space
    clcd_putch(view_data[m][8], LINE2(14));  // Print the first part of the event value
    clcd_putch(view_data[m][9], LINE2(15));  // Print the second part of the event value
}

// Function to display the current time on the CLCD
void display_time(void)
{
    clcd_print(time, LINE2(0));  // Print the time stored in the global `time` array
}

// Function to get the current time from the DS1307 RTC
static void get_time(void)
{
    clock_reg[0] = read_ds1307(HOUR_ADDR);  // Read the hour from DS1307
    clock_reg[1] = read_ds1307(MIN_ADDR);   // Read the minute from DS1307
    clock_reg[2] = read_ds1307(SEC_ADDR);   // Read the second from DS1307

    if (clock_reg[0] & 0x40)  // Check for 12-hour format
    {
        time[0] = '0' + ((clock_reg[0] >> 4) & 0x01);  // Extract the hour tens digit
        time[1] = '0' + (clock_reg[0] & 0x0F);  // Extract the hour ones digit
    }
    else
    {
        time[0] = '0' + ((clock_reg[0] >> 4) & 0x03);  // Extract the hour tens digit
        time[1] = '0' + (clock_reg[0] & 0x0F);  // Extract the hour ones digit
    }
    time[2] = ':';  // Separator between hour and minute
    time[3] = '0' + ((clock_reg[1] >> 4) & 0x0F);  // Extract the minute tens digit
    time[4] = '0' + (clock_reg[1] & 0x0F);  // Extract the minute ones digit
    time[5] = ':';  // Separator between minute and second
    time[6] = '0' + ((clock_reg[2] >> 4) & 0x0F);  // Extract the second tens digit
    time[7] = '0' + (clock_reg[2] & 0x0F);  // Extract the second ones digit
    time[8] = '\0';  // Null-terminate the time string
}

// Function to download the stored data to the terminal
void download_data(char view_data[][11], int total_event)
{
    int k;
    if (total_event < 10)
        k = total_event;  // If total events are less than 10, use that as the count
    else
        k = 10;  // If total events are 10 or more, set count to 10

    puts("#     TIME     EV SP");  // Print the header
    puts("\n\r");  // Newline after the header
    for (int i = 0; i < k; i++)  // Loop through each event
    {
        putch(i + '0');  // Print the index of the event
        puts("   ");
        putch(view_data[i][0]);  // Print the first digit of the hour
        putch(view_data[i][1]);  // Print the second digit of the hour
        putch(';');  // Separator
        putch(view_data[i][2]);  // Print the first digit of the minute
        putch(view_data[i][3]);  // Print the second digit of the minute
        putch(';');  // Separator
        putch(view_data[i][4]);  // Print the first digit of the second
        putch(view_data[i][5]);  // Print the second digit of the second
        puts("   ");  // Space after the second
        putch(view_data[i][6]);  // Print the first character of the gear state
        putch(view_data[i][7]);  // Print the second character of the gear state
        puts(" ");  // Space after the gear state
        putch(view_data[i][8]);  // Print the first part of the event value
        putch(view_data[i][9]);  // Print the second part of the event value
        puts("\n\r");  // Newline after each event
    }
}
