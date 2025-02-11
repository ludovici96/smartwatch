#include "mbed.h"
#include <DFRobot_RGBLCD.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <HTS221Sensor.h>
#include <iostream>
#include <string>
#include <type_traits>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

DigitalIn button1(PC_1, PullUp);
DigitalIn button2(PC_2, PullUp);
DigitalIn button3(PC_4, PullUp);
DigitalIn button4(PC_5, PullUp);

PwmOut piezo(PA_7);

DFRobot_RGBLCD lcd(16, 2, D14, D15);

DevI2C i2c(PB_11, PB_10);
HTS221Sensor sensor(&i2c);

nsapi_size_or_error_t send_request (Socket *socket, const char *request) {
   
    if (socket == nullptr || request == nullptr) {
        printf("Invalid function arguments\n");
        return NSAPI_ERROR_PARAMETER;
    }

    nsapi_size_t bytes_to_send = strlen(request);
    nsapi_size_or_error_t bytes_sendt = 0;

    printf("Sending message: \n%s", request);

    while (bytes_to_send) {
        bytes_sendt = socket->send(request + bytes_sendt, bytes_to_send);

        if (bytes_sendt < 0) {
            return bytes_sendt;
        }
        else {
            printf("Sendt %d bytes\n", bytes_sendt);
        }

        bytes_to_send -= bytes_sendt;
    }

    printf("Complete message sent\n");
    return 0;
}

nsapi_size_or_error_t read_response (Socket *socket, char *buffer, int buffer_lenght) {

    if (socket == nullptr || buffer == nullptr || buffer_lenght < 1) {
        printf("Invalid function arguments\n");
        return NSAPI_ERROR_PARAMETER;
    }

    int remainding_bytes = buffer_lenght;
    int recieved_bytes = 0;

    while (remainding_bytes > 0) {

        nsapi_size_or_error_t result = socket->recv(buffer + recieved_bytes, remainding_bytes);

        if (result == 0) {
            break;
        }

        if (result < 0) {
            return result;
        }

        recieved_bytes += result;
        remainding_bytes -= result;
    }

    printf("\nRecieved %d bytes:\n%.*s\n", recieved_bytes, strstr(buffer, "\n") -buffer, buffer);
    return recieved_bytes;
}

float json_parse_temp (char *data) {

    json object = json::parse(data, nullptr, false);

    if (object.is_discarded()) {
        printf("Invalid input\n");
        return 0;
    }
    else {
        printf("Valid input\n");

        if (object["current"]["temp_c"].is_number_float()) {
            float response = object["current"]["temp_c"];

            return response;
        }
    }
    return 0;
}

string json_parse_status (char *data) {

    json object = json::parse(data, nullptr, false);

    if (object.is_discarded()) {
        printf("Invalid input\n");
        return "invalid input";
    }
    else {
        printf("Valid input\n");

        if (object["current"]["condition"]["text"].is_string()) {
            string response = object["current"]["condition"]["text"];
            
            return response.c_str();
        }
    }
    return "Error";
}

time_t json_parse_epoch(char *data) {
    
    json object = json::parse(data, nullptr, false);

    if (object.is_discarded()) {
        printf("Invalid input\n");
    }
    else {
        printf("Valid input\n");

        if (object["UnixTimeStamp"].is_string()) {
            string unixt = object["UnixTimeStamp"];
            int unixtime = stoi(unixt);

            return unixtime;
        }
    }
    return 0;
}

int main() {

    int i = 0, j = 0, k = 0;
    time_t unixtime, start, end, elapsed = 6;
    time_t w_start = 0, w_end, w_elapsed = 301, n_start = 0, n_end, n_elapsed = 301;
    float w_temp;
    string w_status, news, news2, news3;
    bool alarm_toggle = false;
    int hours = 0, minutes = 0, alarm_h = 7, alarm_m = 30;
    time_t now;
    tm* timeee;
 
    // Buzzer settings
    float buzz = 0.5f;
    int hz = 1000;
    float cycle = 1/(float)hz;
    piezo.period(cycle);

    lcd.init();
    lcd.display();
    lcd.clear();
    lcd.noCursor();
    lcd.printf("     0%i:0%i", hours, minutes);

    NetworkInterface *network = NetworkInterface::get_default_instance();

    if (!network) {
        printf("Failed to get default network interface\n");
        while (1);
    }

    nsapi_size_or_error_t result;

    do {
        printf("Connecting to the network...\n");
        result = network->connect();

        if (result != 0) {
            printf("Failed to connect to network: %d\n", result);
        }
    } while (result != 0);

    SocketAddress address;
    network->get_ip_address(&address);

    printf("Connected to WLAN and got IP address %s\n", address.get_ip_address());

    TCPSocket *socket = new TCPSocket;

    if (socket == nullptr) {
        printf("Failed to allocate socket instance\n");
    }

    socket->open(network);

    const char *host = "showcase.api.linx.twenty57.net";
    result = network->gethostbyname(host, &address);

    if (result != 0) {
        printf("Failed to get IP address of host %s: %d\n", host, result);
        socket->close();
    }
    else
        printf("IP adress of server %s is %s\n", host, address.get_ip_address());

    address.set_port(80);

    result = socket->connect(address);

    if (result != 0) {
        printf("Failed to connect to server at %s: %d\n", host, result);
        socket->close();
    }
    else 
        printf("Successfully connected to server %s\n", host);
            
    const char request[] = "GET /UnixTime/tounixtimestamp?datetime=now HTTP/1.1\r\nHost: showcase.api.linx.twenty57.net\r\nConnection: close\r\n\r\n";
            
    result = send_request(socket, request);

    if (result != 0) {
        printf("Failed to send request: %d\n", result);
        socket->close();
    }

    static constexpr size_t HTTP_RESPONSE_BUF_SIZE = 2000;
    static char response[HTTP_RESPONSE_BUF_SIZE];

    result = read_response(socket, response, HTTP_RESPONSE_BUF_SIZE);

    if (result < 0) {
        printf("Failed to read response: %d\n", result);
        socket->close();
    }
    else {
        response[result] = '\0';
        printf("\nThe HTTP GET response: \n%s\n", response);
            
        socket->close();

        char *json_start = strchr(response, '{');

        if (json_start == nullptr) {
            printf("Start bracket not found\n");
        }

        uint32_t json_size = strlen(response);

        while (json_size > 0) {
                
            if (response[json_size] == '}') {
                break;
            }
            else {
                response[json_size] = '\0';
            }
                json_size--;
        }

        if (json_size == 0 || (response + json_size) < json_start) {
            printf("No endling bracket found\n");
        }
                
        unixtime = json_parse_epoch(json_start);

        delete socket;

        lcd.clear();
            
        printf("Unixtime: %i\n", unixtime);

        lcd.setCursor(0, 0);
        lcd.printf("Unix epoch time: ");

        lcd.setCursor(0, 1);
        lcd.printf("%i", unixtime);
        thread_sleep_for(5000);

        set_time(unixtime + (3600*2));
    }

    while (true) {

        // Next page
        if (button1.read() == 0) {
            i++;
            elapsed = 5;
        }

        // Homepage
        if (button2.read() == 0) {
            i = 0;
            elapsed = 5;
        }

        // Set alarm
        if (button3.read() == 0) {
            k = 1;
        }

        // Toggle alarm
        if (button4.read() == 0) {
            alarm_toggle = !alarm_toggle;
        }

        // Real time clock
        if (i == 0 && elapsed >= 4) {

            time(&start);

            now = time(NULL);

            timeee = localtime(&now);

            hours = timeee->tm_hour;
            minutes = timeee->tm_min;

            printf("\n%i:%i\n", hours, minutes);

            lcd.clear();
            lcd.setCursor(0, 0);
            
            if (hours >= 10 && minutes < 10) {
                lcd.printf("     %i:0%i", hours, minutes);
            }
            else if (hours < 10 && minutes >= 10){
                lcd.printf("     0%i:%i", hours, minutes);
            }
            else if (hours < 10 && minutes < 10) {
                lcd.printf("     0%i:0%i", hours, minutes);
            }
            else {
                lcd.printf("     %i:%i", hours, minutes);
           }
        }

        // Show alarm on home screen if toggled
        if (i == 0) {

            if (alarm_toggle) {
                
                lcd.setCursor(0, 1);
                if (alarm_h < 10 && alarm_m >= 10) {
                    lcd.printf("Alarm  ON  0%i:%i", alarm_h, alarm_m);
                }
                else if (alarm_h >= 10 && alarm_m < 10) {
                    lcd.printf("Alarm  ON  %i:0%i", alarm_h, alarm_m);
                }
                else if (alarm_h < 10 && alarm_m < 10) {
                    lcd.printf("Alarm  ON  0%i:0%i", alarm_h, alarm_m);
                }
                else {
                    lcd.printf("Alarm  ON  %i:%i", alarm_h, alarm_m);
                }
            }
            else {
                
                lcd.setCursor(0, 1);
                if (alarm_h < 10 && alarm_m >= 10) {
                    lcd.printf("Alarm OFF  0%i:%i", alarm_h, alarm_m);
                }
                else if (alarm_h >= 10 && alarm_m < 10) {
                    lcd.printf("Alarm OFF  %i:0%i", alarm_h, alarm_m);
                }
                else if (alarm_h < 10 && alarm_m < 10) {
                    lcd.printf("Alarm OFF  0%i:0%i", alarm_h, alarm_m);
                }
                else {
                    lcd.printf("Alarm OFF  %i:%i", alarm_h, alarm_m);
                }
            }
        }

        // Current temperature and humidity
        if (i == 1 && elapsed >= 4) {

            time(&start);

            float hum;
            float temp;

            sensor.get_humidity(&hum);
            sensor.get_temperature(&temp);

            lcd.clear();

            lcd.setCursor(0, 0);
            lcd.printf("Temp: %.1f C", temp);

            lcd.setCursor(0, 1);
            lcd.printf("Hum:  %.1f %%", hum);

            printf("temp: %.1f C\nhum: %.1f%%\n", temp, hum);

            now = time(NULL);

            timeee = localtime(&now);

            hours = timeee->tm_hour;
            minutes = timeee->tm_min;
        } 

        // Time and date
        if (i == 2 && elapsed >= 4) {

            time(&start);

            now = time(NULL);

            char* dato = ctime(&now);

            timeee = localtime(&now);

            hours = timeee->tm_hour;
            minutes = timeee->tm_min;

            lcd.clear();
            lcd.noCursor();
            lcd.printf("%s", dato);

            printf("%s\n", dato);  
        }

        // Weather forecast
        if (i == 3 && elapsed >= 4) {

            time(&start);
            
            now = time(NULL);
            timeee = localtime(&now);
            hours = timeee->tm_hour;
            minutes = timeee->tm_min;

            if (w_elapsed >= 300) {

                printf("Getting current weather info...\nMight take a few seconds :)\n");
                lcd.clear();
                lcd.noCursor();
                lcd.printf("Loading weather");

                TCPSocket *socket = new TCPSocket;

                if (socket == nullptr) {
                    printf("Failed to allocate socket instance\n");
                }

                socket->open(network);

                const char *host = "api.weatherapi.com";
                result = network->gethostbyname(host, &address);

                if (result != 0) {
                    printf("Failed to get IP address of host %s: %d\n", host, result);
                    socket->close();
                    i = 0;
                }
                else {
                    printf("IP adress of server %s is %s\n", host, address.get_ip_address());
                }

                address.set_port(80);

                result = socket->connect(address);

                if (result != 0) {
                    printf("Failed to connect to server at %s: %d\n", host, result);
                    socket->close();
                }
                else 
                    printf("Successfully connected to server %s\n", host);
                
                const char request[] = "GET /v1/current.json?key=d7c4d7e173cf4ab7982100657222904&q=auto:ip&aqi=no HTTP/1.1\r\nHost: api.weatherapi.com\r\nConnection: close\r\n\r\n";
                
                result = send_request(socket, request);

                if (result != 0) {
                    printf("Failet to send request: %d\n", result);
                    socket->close();
                }

                static constexpr size_t HTTP_RESPONSE_BUF_SIZE = 3000;
                static char response[HTTP_RESPONSE_BUF_SIZE];

                result = read_response(socket, response, HTTP_RESPONSE_BUF_SIZE);

                if (result < 0) {
                    printf("Failed to read response: %d\n", result);
                    socket->close();
                }
                else {
                    response[result] = '\0';
                    printf("\nThe HTTP GET response: \n%s\n", response);
                
                    socket->close();

                    char *json_start = strchr(response, '{');

                    if (json_start == nullptr) {
                        printf("Start bracket not found\n");
                    }

                    uint32_t json_size = strlen(response);

                    while (json_size > 0) {
                    
                        if (response[json_size] == '}') {
                            break;
                        }
                        else {
                            response[json_size] = '\0';
                        }
                        json_size--;
                    }

                    if (json_size == 0 || (response + json_size) < json_start) {
                        printf("No endling bracket found\n");
                    }

                    w_temp = json_parse_temp(json_start);
                    w_status = json_parse_status(json_start);
                    
                    delete socket;
                    
                    time(&w_start);
                }
            }

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.printf("%s", w_status.c_str());
            lcd.setCursor(0, 1);
            lcd.printf("%.1f degrees", w_temp);

            printf("Status: %s\n", w_status.c_str());
            printf("%.1f degrees\n", w_temp);
        }

        if (i == 4 && elapsed >= 4) {

            time(&start);
            now = time(NULL);

            timeee = localtime(&now);

            hours = timeee->tm_hour;
            minutes = timeee->tm_min;

            if (n_elapsed >= 300) {
                
                printf("Loading news...\n");
                lcd.clear();
                lcd.noCursor();
                lcd.printf("Loading news");

                TCPSocket *socket = new TCPSocket;

                if (socket == nullptr) {
                    printf("Failed to allocate socket instance\n");
                }

                socket->open(network);

                const char *host = "feeds.bbci.co.uk";
                result = network->gethostbyname(host, &address);

                if (result != 0) {
                    printf("Failed to get IP address of host %s: %d\n", host, result);
                    socket->close();
                    i = 0;
                }
                else {
                    printf("IP adress of server %s is %s\n", host, address.get_ip_address());
                }

                address.set_port(80);

                result = socket->connect(address);

                if (result != 0) {
                    printf("Failed to connect to server at %s: %d\n", host, result);
                    socket->close();
                }
                else 
                    printf("Successfully connected to server %s\n", host);
                    
                const char request[] = "GET /news/world/rss.xml HTTP/1.1\r\nHost: feeds.bbci.co.uk\r\nConnection: close\r\n\r\n";
                    
                result = send_request(socket, request);

                if (result != 0) {
                    printf("Failet to send request: %d\n", result);
                    socket->close();
                }

                static constexpr size_t HTTP_RESPONSE_BUF_SIZE = 3000;
                static char response[HTTP_RESPONSE_BUF_SIZE];

                result = read_response(socket, response, HTTP_RESPONSE_BUF_SIZE);

                if (result < 0) {
                    printf("Failed to read response: %d\n", result);
                    socket->close();
                }
                else {
                    response[result] = '\0';
                    printf("\nThe HTTP GET response: \n%s\n", response);
                    
                    socket->close();

                    string test = response;
                    test = test.erase(0, test.find("<title><![CDATA[") + 16);
                    news = test.erase(0, test.find("<title><![CDATA[") + 16);
                    string test3 = news;
                    news2 = test3.erase(0, test3.find("<title><![CDATA[") + 16);
                    string test2 = news2;
                    news3 = test2.erase(0, test2.find("<title><![CDATA[") + 16);

                    news = news.erase(news.find("]]></title>"));
                    news2 = news2.erase(news2.find("]]></title>"));
                    news3 = news3.erase(news3.find("]]></title>"));

                    printf("\n\n%s\n", news.c_str());
                    printf("\n\n%s\n", news2.c_str());
                    printf("\n\n%s\n", news3.c_str());

                    delete socket;
                        
                    time(&n_start);
                }
            }
            
            int lone = news.length();
            char one[lone + 1];
            strcpy(one, news.c_str());

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.printf("Latest news: ");

            for (int i = 0; i < lone - 15; i++) {

                for (int j = 0; j < 16; j++) {
                    lcd.setCursor(0+j, 1);
                    lcd.printf("%c", one[i + j]);
                }

                if (button1.read() == 0) {
                    break;
                }
                thread_sleep_for(400);
            }
            thread_sleep_for(750);

            int ltwo = news2.length();
            char two[ltwo + 1];
            strcpy(two, news2.c_str());

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.printf("Latest news: ");

            for (int i = 0; i < ltwo - 15; i++) {

                for (int j = 0; j < 16; j++) {
                    lcd.setCursor(0+j, 1);
                    lcd.printf("%c", two[i + j]);
                }

                if (button1.read() == 0) {
                    break;
                }
                thread_sleep_for(400);
            }
            thread_sleep_for(750);

            int lthree = news3.length();
            char three[lthree + 1];
            strcpy(three, news3.c_str());

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.printf("Latest news: ");

            for (int i = 0; i < lthree - 15; i++) {

                for (int j = 0; j < 16; j++) {
                    lcd.setCursor(0+j, 1);
                    lcd.printf("%c", three[i + j]);
                }

                if (button1.read() == 0) {
                    break;
                }
                thread_sleep_for(400);
            }
            thread_sleep_for(750);
            lcd.clear();
            
            i++;

            now = time(NULL);
            timeee = localtime(&now);

            hours = timeee->tm_hour;
            minutes = timeee->tm_min;
        }

        if (k == 1) {
            
            bool first = true;
            bool second = false;

            printf("Alarm selection\n");

            lcd.clear();

            while (first) {

                if (button1.read() == 0) {
                   alarm_h++;
                }
                if (button3.read() == 0) {
                   alarm_h--;
                } 
                if (alarm_h > 23) {
                    alarm_h = 0;
                }
                if (alarm_h < 0) {
                    alarm_h = 23;
                }
                if (button2.read() == 0) {
                    second = true;
                }
                
                lcd.setCursor(0, 0);
                lcd.printf("Set alarm:");
                lcd.setCursor(0, 1);
                if (alarm_h < 10 && alarm_m >= 10) {
                    lcd.printf("Alarm      0%i:%i", alarm_h, alarm_m);
                }
                else if (alarm_h >= 10 && alarm_m < 10) {
                    lcd.printf("Alarm      %i:0%i", alarm_h, alarm_m);
                }
                else if (alarm_h < 10 && alarm_m < 10) {
                    lcd.printf("Alarm      0%i:0%i", alarm_h, alarm_m);
                }
                else if (alarm_h >= 10 && alarm_m >= 10) {  
                    lcd.printf("Alarm      %i:%i", alarm_h, alarm_m);
                }
                
                while (second) {

                    if (button1.read() == 0) {
                        alarm_m++;
                    }
                    if (button3.read() == 0) {
                        alarm_m--;
                    }
                    if (alarm_m > 59) {
                        alarm_m = 0;
                    }
                    if (alarm_m < 0) {
                        alarm_m = 59;
                    }
                    
                    lcd.setCursor(0, 0);
                    lcd.printf("Set alarm:");
                    lcd.setCursor(0, 1);
                    if (alarm_h < 10 && alarm_m >= 10) {
                        lcd.printf("Alarm      0%i:%i", alarm_h, alarm_m);
                    }
                    else if (alarm_h >= 10 && alarm_m < 10) {
                        lcd.printf("Alarm      %i:0%i", alarm_h, alarm_m);
                    }
                    else if (alarm_h < 10 && alarm_m < 10) {
                        lcd.printf("Alarm      0%i:0%i", alarm_h, alarm_m);
                    }
                    else if (alarm_h >= 10 && alarm_m >= 10) {  
                        lcd.printf("Alarm      %i:%i", alarm_h, alarm_m);
                    }

                    if (button2.read() == 0) {
                        first = false;
                        second = false;
                        alarm_toggle = true;
                        k = 0;
                        i = 0;
                    }
                }
            }
        }

        time(&end);
        elapsed = end - start;

        if (elapsed > 5) {
            elapsed = 0;
        }

        if (w_start != 0) {
            time(&w_end);
            w_elapsed = w_end - w_start;
        }

        if (n_start != 0) {
            time(&n_end);
            n_elapsed = n_end - n_start;
        }

        if (i >= 5) {
            i = 0;
            elapsed = 5;
        }

        // Ring alarm
        int r = 0;
        while (alarm_toggle && alarm_h == hours && alarm_m == minutes) {
            
            if (r == 0) {
                lcd.clear();
                r++;
            }
            
            lcd.setCursor(0, 0);
            lcd.printf("Alarm going off!");
            
            piezo.write(buzz);
            
            // Turn off alarm
            if (button1.read() == 0) {
                piezo.write(0);
                alarm_toggle = false;
                elapsed = 5;
            }

            // Snooze alarm
            if (button2.read() == 0) {
                piezo.write(0);
                alarm_m += 5;

                if (alarm_m >= 60) {
                    alarm_m = alarm_m -  60;
                    alarm_h += 1;
                }
                
                elapsed = 5;
            }
        }

        thread_sleep_for(50);    
    }   
}


// Button 1 is for switching through pages
// Button 2 takes you back to the main page
// Button 3 takes you to the alarm settings
// Button 4 toggles alarm ON/OFF

// In alarm settings:
//     Button 1 for +
//     Button 3 for - 
//     Button 2 for confirm

// When alarm is going off:
//     Button 1 to stop alarm
//     Button 2 to snooze alarm 5 minutes