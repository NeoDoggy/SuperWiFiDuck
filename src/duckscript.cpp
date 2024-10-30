/*
   This software is licensed under the MIT License. See the license file for details.
   Source: https://github.com/spacehuhntech/WiFiDuck
*/

#include "duckscript.h"
#include "duckparser.h"
// #include "Vector.h"
#include <cppQueue.h>

#include "config.h"
#include "debug.h"
#include "Queue.h"
#include "spiffs.h"

namespace duckscript {

    File f;
    // Vector<String> prevNmsg;
    // Vector<size_t> prevNmsgS;
    // cppQueue	q(sizeof(String *), 256, FIFO);	// Instantiate queue
    // cppQueue	qs(sizeof(size_t), 256, FIFO);	// Instantiate queue
    Queue<String> q=Queue<String>();
    Queue<size_t> qs=Queue<size_t>();
    uint16_t ql=0;
    int whilecnt=0;
    bool dowhile=0;
    char * prevMessage    { NULL };
    size_t prevMessageLen { 0 };
    String * pStr;
    size_t tmps;

    bool running { false };

    static void runTask(void* parameter) {
        String* fileNamePtr = (String*) parameter;
        String fileName = *fileNamePtr;
        delete fileNamePtr;  // Free the heap memory

        if (fileName.length() > 0) {
            debugf("Run file %s\n", fileName.c_str());
            f       = spiffs::open(fileName);
            running = true;
            nextLine();
        }

        vTaskDelete(NULL);  // Delete this task when done
    }

    void run(String fileName) {
        // Create a heap copy of fileName
        String* fileNamePtr = new String(fileName);

        // Create a new task
        xTaskCreate(
                runTask,               // Function that should be executed
                "runDuckScriptTask",   // Name of the task
                4096,                  // Stack size in words
                fileNamePtr,           // Parameter to pass to the task
                1,                     // Priority
                NULL                   // Task handle
        );
    }

    void nextLine() {
        while (running) {
            if (!f) {
                debugln("File error");
                stopAll();
                return;
            }

            if (!f.available()) {
                debugln("Reached end of file");
                stopAll();
                return;
            }

            char buf[BUFFER_SIZE];
            unsigned int buf_i =  0;
            bool eol = false; // End of line

            // Read a line into the buffer
            while (f.available() && !eol && buf_i < BUFFER_SIZE) {
                uint8_t b = f.peek();
                eol = (b == '\n');
                buf[buf_i] = f.read();
                ++buf_i;
            }

            if (!eol) debugln();


            if (strncmp((char*)buf, "REPEAT", _min(buf_i, 6)) == 0 || strncmp((char*)buf, "REPLAY", _min(buf_i, 6)) == 0) {
                // Extract repeat count if available
                char* pch = strtok((char*)buf + 6, " ");
                int repeatCount = atoi(pch);
                if (repeatCount <= 0) {
                    repeatCount = 1;
                }

                // Repeat the previous command
                for (int i = 0; i < repeatCount; ++i) {
                    if (prevMessage) {
                        duckparser::parse(prevMessage, prevMessageLen);
                    }
                }
            }
            else if (strncmp((char*)buf, "WHILEN", _min(buf_i, 6)) == 0 ){
                // Extract repeat count if available
                char* pch = strtok((char*)buf + 6, " ");
                whilecnt = atoi(pch)<=0?1:atoi(pch);
                q.clear();
                qs.clear();
                // q.clean();
                // qs.clean();
                ql=0;
                dowhile=1;
            }
            else if (strncmp((char*)buf, "ENDWHL", _min(buf_i, 6)) == 0 ){
                Queue<String> tmpq=q;
                Queue<size_t> tmpqs=qs;
                for (int i=0;i<whilecnt;++i) {
                    while(tmpq.count()>0){
                        String tmps=tmpq.pop();
                        if(tmps)duckparser::parse(tmps.c_str(),tmpqs.pop());  
                    }
                }
                q.clear();
                qs.clear();
                whilecnt=0;
                // duckparser::parse(String(ql).c_str(),1);
                ql=0;
                dowhile=0;
            }
            else {
                // Store this as the previous message
                if (prevMessage) free(prevMessage);
                prevMessageLen = buf_i;
                prevMessage = (char*)malloc(prevMessageLen + 1);
                memcpy(prevMessage, buf, buf_i);
                prevMessage[buf_i] = '\0';
                if(dowhile){
                    ql++;
                    String tmpbf(prevMessage);
                    q.push(tmpbf);
                    qs.push(prevMessageLen);
                    // q.push(&tmpbf);
                    // qs.push(&prevMessageLen);
                    // prevNmsg.push_back(tmpbf);
                    // prevNmsgS.push_back(prevMessageLen);
                }
                else{
                    duckparser::parse(buf, buf_i);
                }
            }
            
        }
    }

    void stopAll() {
        if (running) {
            if (f) f.close();
            running = false;
            debugln("Stopped script");
        }
    }

    void stop(String fileName) {
        if (fileName.length() == 0) stopAll();
        else {
            if (running && f && (fileName == currentScript())) {
                f.close();
                running = false;
                debugln("Stopped script");
            }
        }
    }

    bool isRunning() {
        return running;
    }

    String currentScript() {
        if (!running) return String();
        return String(f.name());
    }
}