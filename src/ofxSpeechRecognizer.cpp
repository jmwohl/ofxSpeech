/*
 *  ofxSpeechRecognizer.cpp
 *  openFrameworks
 *
 *  Created by latrokles on 5/11/09.
 *  Copyright 2009 Samurai Hippo. All rights reserved.
 *
 */
#include "ofxSpeechRecognizer.h"
ofEvent<std::string>        ofxSpeechRecognizer::speechRecognizedEvent;

/*
 * Removes leading and trailing blank space from stringToClean.
 */
void cleanUpString(std::string &stringToClean)
{
    if(!stringToClean.empty())
    {
        int firstCharacter      = stringToClean.find_first_not_of(" ");
        int lastCharacter       = stringToClean.find_last_not_of(" ");
        std::string tempString  = stringToClean;
        stringToClean.erase();
        
        stringToClean           = tempString.substr(firstCharacter, (lastCharacter-firstCharacter + 1));
    }
    
}

/*
 * Removes leading space from stringToClean and sets the string to the passed in length
 */
void cleanUpString(std::string &stringToClean, int len)
{
    if(!stringToClean.empty())
    {
        int firstCharacter      = stringToClean.find_first_not_of(" ");
        std::string tempString  = stringToClean;
        stringToClean.erase();
        
        stringToClean           = tempString.substr(firstCharacter, len);
    }
    
}

/*
 * Instantiates an ofxSpeechRecognizer object and sets listening to false
 */
ofxSpeechRecognizer::ofxSpeechRecognizer()
{
    listening = false;
}

ofxSpeechRecognizer::~ofxSpeechRecognizer()
{
    OSErr       errorStatus;
    stopListening();
    
    errorStatus = SRReleaseObject(speechRecognizer);
    errorStatus = SRCloseRecognitionSystem(recognitionSystem);
}
/*
 * Initializes the recognizer, by opening the recognition system, creating a 
 * a recognizer, settting listening parameters, selecting a speech source (for 
 * now just the microphone), and installing an event handler for OSX speech 
 * event.
 *
 * Source types:
 * enum {
 *      kSRDefaultSpeechSource = 0,
 *      kSRLiveDesktopSpeechSource = 'dklv',
 *      kSRCanned22kHzSpeechSource = 'ca22'
 * };
 *
 * @param source The source for the recognizer
 */
void ofxSpeechRecognizer::initRecognizer(OSType source)
{
    OSErr errorStatus;
    
    //-- Install eventHandler
    errorStatus = AEInstallEventHandler(kAESpeechSuite, kAESpeechDone, NewAEEventHandlerUPP((AEEventHandlerProcPtr)handleSpeechDone), 0, false);
    
    //-- Open the recognition system
    errorStatus = SROpenRecognitionSystem(&recognitionSystem, kSRDefaultRecognitionSystemID);
    
    //-- Override system defaults
    if(false && !errorStatus)
    {
        short feedbackModes = kSRNoFeedbackNoListenModes;
        errorStatus         = SRSetProperty(recognitionSystem, kSRFeedbackAndListeningModes, &feedbackModes, sizeof(feedbackModes));
    }
    
    //-- Create a recognizer that uses the default speech source (microphone)
    if(!errorStatus)
    {
        errorStatus = SRNewRecognizer(recognitionSystem, &speechRecognizer, source);
    }
}

/*
 * Init recognizer with source kSRCanned22kHzSpeechSource
 */
void ofxSpeechRecognizer::initRecognizerFromFileSource() {
    initRecognizer(kSRCanned22kHzSpeechSource);
}

/*
 * Init recognizer with source kSRDefaultSpeechSource
 */
void ofxSpeechRecognizer::initRecognizerFromLiveInputSource() {
    initRecognizer(kSRDefaultSpeechSource);
}

/*
 * creates a language model and adds the words contained in the wordsToRecognize
 * vector to said language model.
 */
void ofxSpeechRecognizer::loadDictionary(const std::vector<std::string> &wordsToRecognize)
{
    //-- If wordsToRecognize is empty, don't bother
    if(!wordsToRecognize.empty())
    {
        OSErr               errorStatus;
        SRLanguageModel     speechLanguageModel;
        const char *        languageModelName = "<Single Model>";
        
        vocabulary = wordsToRecognize;
        
        //-- Make language model
        errorStatus = SRNewLanguageModel(recognitionSystem, &speechLanguageModel, languageModelName, strlen(languageModelName));
        
        //-- Add words to the language model
        if(!errorStatus)
        {
            for(int wordIndex = 0; wordIndex < vocabulary.size(); wordIndex++)
            {
                //-- SRAddText requires c strings so need to convert, or maybe we should 
                //-- pass a vector of const char * to begin with, something to think about.
                const char * word = vocabulary[wordIndex].c_str();
                errorStatus = SRAddText(speechLanguageModel, word, strlen(word), 0);
            }
        }
        
        //-- If there's an error adding the words, release the language model
        if(errorStatus)
        {
            cout << "early error" << endl;
            SRReleaseObject(speechLanguageModel);
        }
        //-- else set the language model in the system and release model
        else
        {
            cout << "no early err" << endl;
            errorStatus = SRSetLanguageModel(speechRecognizer, speechLanguageModel);
            SRReleaseObject(speechLanguageModel);
        }
    }
}

/*
 * Opens dictionaryFilename, reads its contents and loads them into a vector of
 * strings to them pass them along to addDictionary.
 */
void ofxSpeechRecognizer::loadDictionaryFromFile(std::string dictionaryFilename)
{
    dictionaryFilename          = ofToDataPath(dictionaryFilename);
    std::vector<std::string>    dictionary;
    std::string                 dictionaryWord;
    std::ifstream               dictionaryFile(dictionaryFilename.data(), std::ios_base::in);
    
    //-- Iterate through file and add each line to the dictionary vector
    while(getline(dictionaryFile, dictionaryWord, '\n'))
    {
        dictionary.push_back(dictionaryWord);
    }
    dictionaryFile.close();
    
    //-- Already wrote a function to add a vector of strings, may as well use it.
    /* It may be two inefficient since I am iterating through the same contents 
     * twice, once here and the other in addDictionary, may be worth it just to
     * use the same code here and be done with it, or just refactor it in a way
     * that I can do keep the language model creation code in one place. 
     * Something to consider.
     */
    loadDictionary(dictionary);
}


void ofxSpeechRecognizer::loadAudioFile(std::string audioFilename) {
    OSErr err = 0;
    
    audioFilename = ofToDataPath(audioFilename);
    ofFile audioFile = ofFile(audioFilename);
    audioFilename = audioFile.getAbsolutePath();
    const char *filePath = "/Volumes/Big Bro/Users/jon/OF/0.8.0/apps/myApps/VoiceToTextTest/bin/data/Text.aiff";
    
    cout << audioFilename.c_str() << endl;
    
    Size len = audioFile.getSize();
    
    FSRef ref;
    FSSpec fsspec;
    OSStatus os_status = FSPathMakeRef((UInt8 *)filePath, &ref, NULL);
    SRSpeechObject tempObj;
    
    long count;
    
//    err = SRCountItems(*so, &count);
    
    if (!err) {
        cout << count << endl;
    }
    
    
    if (os_status == noErr) {
        cout << "Success" << endl;
        err = FSGetCatalogInfo (&ref, kFSCatInfoNone, NULL, NULL, &fsspec, NULL);
        if (!err) {
            cout << "no error here ... " << endl;
        }
    }
    
    
    err = SRSetProperty(speechRecognizer, kSRReadAudioFSSpec, &fsspec, sizeof(fsspec));
    
    if(!err)
    {
        cout << "no error!" << endl;
    } else {
        cout << "error." << endl;
    }
}

/*
 * Starts the recognizer if it's not already running
 */
void ofxSpeechRecognizer::startListening()
{
    if(!listening)
    {
        OSErr statusError;
        statusError = SRStartListening(speechRecognizer);
        listening = true;
    }
}

/*
 *  Stops the recoginzer if it's running
 */
void ofxSpeechRecognizer::stopListening()
{
    if(listening)
    {
        OSErr statusError;
        statusError = SRStopListening(speechRecognizer);
        listening = false;
    }
}

/*
 * Returns the status of the reconizer
 */
bool ofxSpeechRecognizer::isListening()
{
    return listening;
}

/*
 *  Handles the speechDone event as specified by the OSX speech recognition
 *  manager reference. It extracts the string that was recognized, converts it
 *  into a nice C++ string, cleans it up, and notifies any listeners of the 
 *  word detected.
 */
pascal OSErr ofxSpeechRecognizer::handleSpeechDone(const AppleEvent *theAEevt, AppleEvent* reply, long refcon)
{
    long                actualSize;
    DescType            actualType;
    OSErr               errorStatus = 0, recStatus = 0;
    SRRecognitionResult recognitionResult;
    char                resultStr[MAX_RECOGNITION_LEN];
    Size                len;
    
    //-- Check status of the speech recognizer
    errorStatus = AEGetParamPtr(theAEevt, keySRSpeechStatus, typeShortInteger, &actualType, (Ptr)&recStatus, sizeof(errorStatus), &actualSize);
    
    //-- Get the recognition result object from the recognizer
    if(!errorStatus && !recStatus)
    {
        errorStatus = AEGetParamPtr(theAEevt, keySRSpeechResult, typeSRSpeechResult, &actualType, (Ptr)&recognitionResult, sizeof(SRRecognitionResult), &actualSize);
    }
    
    //-- Extract the words recognized in the result object
    if(!errorStatus)
    {
        len = MAX_RECOGNITION_LEN - 1;
        errorStatus = SRGetProperty(recognitionResult, kSRTEXTFormat, &resultStr, &len);
        
        if(!errorStatus)
        {
            //-- We are done with the recognition result object, we can release it now
            SRReleaseObject(recognitionResult);
            
            std::string wordRecognized = std::string(resultStr);
            
            /*
             * Since the resulting string from the recgontionResult is 255 characters 
             * in length, we want to strip out any leading or trailing blank space so 
             * that we can compare it easily in the event handler inside of testApp
             */
            cleanUpString(wordRecognized, len);
            
            //-- Notify our speechRecognizedEvent listeners.
            ofNotifyEvent(speechRecognizedEvent, wordRecognized);
        }
    }
}