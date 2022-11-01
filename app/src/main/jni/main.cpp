#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <unistd.h>
#include <stdio.h>

#define USE_JNI 1
// Do we want to showcase direct JNI/NDK interaction?
// Undefine this to get real cross-platform code.
// Uncomment this to try JNI access; this seems to be broken in latest NDKs
//#define USE_JNI

#if defined(USE_JNI)
// These headers are only needed for direct NDK/JDK interaction
#include <jni.h>
#include <android/native_activity.h>
// Since we want to get the native activity from SFML, we'll have to use an
// extra header here:
#include <SFML/System/NativeActivity.hpp>
#include <string>
#include <android/log.h>
#include <sys/stat.h>
// NDK/JNI sub example - call Java code from native code
int vibrate(sf::Time duration)
{
    // First we'll need the native activity handle
    ANativeActivity *activity = sf::getNativeActivity();

    // Retrieve the JVM and JNI environment
    JavaVM* vm = activity->vm;
    JNIEnv* env = activity->env;

    // First, attach this thread to the main thread
    JavaVMAttachArgs attachargs;
    attachargs.version = JNI_VERSION_1_6;
    attachargs.name = "NativeThread";
    attachargs.group = NULL;
    jint res = vm->AttachCurrentThread(&env, &attachargs);

    if (res == JNI_ERR)
        return EXIT_FAILURE;

    // Retrieve class information
    jclass natact = env->FindClass("android/app/NativeActivity");
    jclass context = env->FindClass("android/content/Context");

    // Get the value of a constant
    jfieldID fid = env->GetStaticFieldID(context, "VIBRATOR_SERVICE", "Ljava/lang/String;");
    jobject svcstr = env->GetStaticObjectField(context, fid);

    // Get the method 'getSystemService' and call it
    jmethodID getss = env->GetMethodID(natact, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject vib_obj = env->CallObjectMethod(activity->clazz, getss, svcstr);

    // Get the object's class and retrieve the member name
    jclass vib_cls = env->GetObjectClass(vib_obj);
    jmethodID vibrate = env->GetMethodID(vib_cls, "vibrate", "(J)V");

    // Determine the timeframe
    jlong length = duration.asMilliseconds();

    // Bzzz!
    env->CallVoidMethod(vib_obj, vibrate, length);

    // Free references
    env->DeleteLocalRef(vib_obj);
    env->DeleteLocalRef(vib_cls);
    env->DeleteLocalRef(svcstr);
    env->DeleteLocalRef(context);
    env->DeleteLocalRef(natact);

    // Detach thread again
    vm->DetachCurrentThread();
}

void writeScore2File(std::string fileName, int score)
{
    ANativeActivity* nativeActivity = sf::getNativeActivity();
    const char* internalPath = nativeActivity->internalDataPath;
    std::string dataPath(internalPath);
    // internalDataPath points directly to the files/ directory
    std::string configFile = dataPath +"/" + fileName;//"/app_config.xml";

    // sometimes if this is the first time we run the app
    // then we need to create the internal storage "files" directory
    struct stat sb;
    int32_t res = stat(dataPath.c_str(), &sb);
    if (0 == res && sb.st_mode & S_IFDIR)
    {
        __android_log_print(ANDROID_LOG_INFO, "'files/' dir already in app's internal data storage.", "");
    }
    else if (ENOENT == errno)
    {
        res = mkdir(dataPath.c_str(), 0770);
    }

    if (0 == res)
    {
        // test to see if the config file is already present
        res = stat(configFile.c_str(), &sb);
        if (0 == res && sb.st_mode & S_IFREG)
        {
            __android_log_print(ANDROID_LOG_INFO, "Application config file already present", "extra: %d", 1);
        }
        else {
            __android_log_print(ANDROID_LOG_INFO,
                                "Application config file does not exist. Creating it ...", "%d", 1);
            // read our application config file from the assets inside the apk
            // save the config file contents in the application's internal storage
            __android_log_print(ANDROID_LOG_INFO, "Reading config file using the asset manager.\n",
                                "extra: %d", 1);
        }
        /*
            AAssetManager* assetManager = nativeActivity->assetManager;
            AAsset* configFileAsset = AAssetManager_open(assetManager, "app_config.xml", AASSET_MODE_BUFFER);
            const void* configData = AAsset_getBuffer(configFileAsset);
            const off_t configLen = AAsset_getLength(configFileAsset);
        */
        FILE* appConfigFile = std::fopen(configFile.c_str(), "wb");
        if (NULL == appConfigFile)
        {
             __android_log_print(ANDROID_LOG_INFO,"Could not create app configuration file.\n", "");
        }
        else
        {
            __android_log_print(ANDROID_LOG_INFO,"App config file created successfully. Writing config data ...\n", "");
            res = fwrite(&score, sizeof(score), 1, appConfigFile);
                /*
                if (configLen != res)
                {
                    __android_log_print(ANDROID_LOG_INFO,"Error generating app configuration file.\n", "");
                }*/
        }
        std::fclose(appConfigFile);
            // AAsset_close(configFileAsset);
    }
}


void loadScoreFromFile(std::string fileName, int &score)
{
    ANativeActivity* nativeActivity = sf::getNativeActivity();
    const char* internalPath = nativeActivity->internalDataPath;
    std::string dataPath(internalPath);
    // internalDataPath points directly to the files/ directory
    std::string configFile = dataPath +"/" + fileName;//"/app_config.xml";

    // sometimes if this is the first time we run the app
    // then we need to create the internal storage "files" directory
    struct stat sb;
    int32_t res = stat(dataPath.c_str(), &sb);
    if (0 == res && sb.st_mode & S_IFDIR)
    {
        __android_log_print(ANDROID_LOG_INFO, "'files/' dir already in app's internal data storage.", "");
    }
    else if (ENOENT == errno)
    {
        res = mkdir(dataPath.c_str(), 0770);
    }

    if (0 == res)
    {
        // test to see if the config file is already present
        res = stat(configFile.c_str(), &sb);
        if (0 == res && sb.st_mode & S_IFREG)
        {
            __android_log_print(ANDROID_LOG_INFO, "Application config file already present", "extra: %d", 1);
            FILE* appConfigFile = std::fopen(configFile.c_str(), "rb");
            if (NULL == appConfigFile)
            {
                __android_log_print(ANDROID_LOG_INFO,"Could not create app configuration file.\n", "");
            }
            else
            {
                __android_log_print(ANDROID_LOG_INFO,"App config file created successfully. Writing config data ...\n", "");
                res = std::fread(&score, sizeof(score), 1, appConfigFile);

            }
            fclose(appConfigFile);
        }
        else
        {
            __android_log_print(ANDROID_LOG_INFO, "Application config file does not exist. Exit...", "%d", 1);
        }
    }
}

#endif

// This is the actual Android example. You don't have to write any platform
// specific code, unless you want to use things not directly exposed.
// ('vibrate()' in this example; undefine 'USE_JNI' above to disable it)

#include <iostream>
#include <SFML/Graphics.hpp>
#include <ctime> // time_t
#include <cstdio>

void isTouched(sf::Sprite spr_Button , sf::Vector2f mouse, float bscale, bool&left, bool&right, bool&up, bool&down) {

    float bw = spr_Button.getTexture()->getSize().x*bscale;
    float bh = spr_Button.getTexture()->getSize().y*bscale;
    float leftx = spr_Button.getPosition().x;
    float lefty = spr_Button.getPosition().y+bh/3;

    if(mouse.x>=leftx && mouse.x<=leftx+bw/3 && mouse.y>=lefty && mouse.y<=lefty+bh/3) {
        left = true;
    }

    float rightx = leftx + bw/3*2;
    float righty = lefty;

    if(mouse.x>=rightx && mouse.x<=rightx+bw/3 && mouse.y>=righty && mouse.y<=righty+bh/3) {
        right = true;
    }

    float upx = spr_Button.getPosition().x + bw/3;
    float upy = spr_Button.getPosition().y;

    if(mouse.x>=upx && mouse.x<=upx+bw/3 && mouse.y>=upy && mouse.y<=upy+bh/3) {
        up = true;
    }
    float downx = spr_Button.getPosition().x + bw/3;
    float downy = spr_Button.getPosition().y + bh/3*2;

    if(mouse.x>=downx && mouse.x<=downx+bw/3 && mouse.y>=downy && mouse.y<=downy+bh/3) {
        down = true;
    }
    return;
}

class Arrows {
private:
    sf::Sprite sprite;
    sf::Texture texture;
    float scale;
    float speedx;
    float speedy;
    sf::Vector2f position;
public:
    Arrows(sf::Sprite sprite_, sf::Texture texture_, float spx, float spy, float scale_) {
        sprite = sprite_;
        texture = texture_;
        speedx = spx;
        speedy = spy;
        scale = scale_;
    }

    void initPosition(int x, int y, int screenWidth, int ScreenHeight) {

        int spriteX = scale*x;
        int spriteY = scale*y;
        int cx = screenWidth/2 - spriteX/2; // shift left 40
        int cy  = spriteY/2; // shift down
        position = (sf::Vector2f(cx, cy));
    }

    void setSpeed(float spx, float spy) {
        speedx = spx;
        speedy = spy;
    }
    void setTexture(sf::Texture txt) {
        texture = txt;
    }
    void setSprite(sf::Sprite spt) {
        sprite = spt;
    }
    void update() {
        position.x +=speedx;
        position.y +=speedy;
    }

    void show(sf::RenderWindow &window) {
        sprite.setTexture(texture);
        sprite.setPosition(position);
        sprite.setScale(scale, scale);// relative scale
        window.draw(sprite);
    }
};

void renderScore(sf::Font font, int score, int maxScore,  int width, sf::RenderWindow&window) {
    sf::Text textScore;
    textScore.setFont(font);
    textScore.setString(std::to_string(score)  + "  max: " + std::to_string(maxScore));

    float length = textScore.getLocalBounds().width;
    textScore.setPosition(((float)(width) / 2 - (float)(length / 2)), 0);
    window.draw(textScore);
}


int gscore = 0;
std::string gfile="config.txt";
int main()
{
    loadScoreFromFile(gfile, gscore);
    //sf::RenderWindow window(sf::VideoMode(width, height), "SFML works!");
    sf::VideoMode screen(sf::VideoMode::getDesktopMode());

    sf::RenderWindow window(screen, "android test");
    // window.setFramerateLimit(30);

    sf::View view = window.getDefaultView();

    int width = view.getSize().x;
    int height = view.getSize().y;
    sf::Color background = sf::Color::Black;

    sf::CircleShape shape(100.f);
    shape.setOrigin(100 , 100);
    shape.setFillColor(sf::Color::Green);

    sf::Texture greenleft,greenright,greendown,greenup;

    sf::Texture redleft,redright,reddown,redup;

    // char apppath[256];
    // getcwd(apppath, 256);
    // printf("%d, %s\n", sizeof(apppath), apppath);
    bool isOk = greenleft.loadFromFile("images/green_left.jpg");
    if(!isOk) {
        printf("cannot load image\r\n");
        return -1;
    }
    greenright.loadFromFile("images/green_right.jpg");
    greendown.loadFromFile("images/green_down.jpg");
    greenup.loadFromFile("images/green_up.jpg");

    redleft.loadFromFile("images/red_left.jpg");
    redright.loadFromFile("images/red_right.jpg");
    reddown.loadFromFile("images/red_down.jpg");
    redup.loadFromFile("images/red_up.jpg");

    sf::Sprite sprite0, sprite1;
    sf::Font font;
    font.loadFromFile("sansation.ttf");

    // button information here
    sf::Sprite spr_Button;
    sf::Texture play_button;
    play_button.loadFromFile("images/arrows.jpg");
    spr_Button.setTexture(play_button);
    float bscale = 0.9;
    spr_Button.setScale(bscale, bscale);
    spr_Button.setPosition(width/2-bscale*spr_Button.getTexture()->getSize().x/2, -150+height-bscale*spr_Button.getTexture()->getSize().y);


    int ranker = 2; // 5: sleep 5 seconds, 2 seconds, and 1 second to indicate speed

    float spx =0;
    float spy = 20;// 0.2; // double speed here

    Arrows arrow = Arrows(sprite0, greenleft, spx, spy, 0.5);
    arrow.initPosition(greenleft.getSize().x, greenleft.getSize().y, width, height);
    sf::Clock clock;
    sf::Time elapsed1 = clock.getElapsedTime();

    int randNum = (rand() % 4); // get 0, 1, 2, 3
    int interval = 8; // 4 seconds // interval = 1; // speed up
    time_t begin,end; // time_t is a datatype to store time values.
    time (&begin); // note time before execution

    // Flags for key pressed
    bool upFlag=false;
    bool downFlag=false;
    bool leftFlag=false;
    bool rightFlag=false;
    int score = 0;
    int total = 0;
    bool lock = false;


    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        // sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        // donot update until new time
        time (&end); // note time after execution
        int difference = difftime(end,begin);

        total = total + difference;
        if(total>240)
        {
            sf::Text textScore;
            textScore.setFont(font);
            textScore.setString(std::to_string(score) + "\n" + "Game Over!");
            textScore.setScale(10, 10);
            float length = textScore.getLocalBounds().width;
            textScore.setPosition(((float)(width) / 2 - (float)(length / 2)), 0);
            window.draw(textScore);
            sleep(10);
            break; // 4 mins' game
        }
        if(difference >= interval || lock) {
            time (&begin);
            randNum = (rand() % 4); // get 0, 1, 2, 3
            lock = false;
            if (randNum==0 || randNum==1) {
                arrow.initPosition(greenleft.getSize().x,greenleft.getSize().x, width, height);
            }
            else {
                arrow.initPosition(greenup.getSize().x,greenup.getSize().x, width, height);
            }
        }

        if(randNum==0) {
            //draw left
            arrow.setTexture(greenleft);
            arrow.setSprite(sprite0);
            if(leftFlag &&!lock) {
                arrow.setTexture(redleft);
                score +=10;
                leftFlag=false;
                lock = true;
                /*
                time_t current;

                time(&current);
                if (difftime(current,begin) < 4) {
                    sf::sleep(sf::seconds(4-difftime(current,begin)));
                }
                */
            }
            // if press any other button, no score even right later/lock it
            /*
            if(rightFlag||upFlag||downFlag) {
                lock = true;
            }*/
        }
        else if(randNum==1) {

            arrow.setTexture(greenright);
            arrow.setSprite(sprite0);
            if(rightFlag&&!lock) {
                arrow.setTexture(redright);
                score +=10;
                rightFlag = false;
                lock = true;
            }

            // if press any other button, then lock it
            /*if(leftFlag||upFlag||downFlag) {
                lock = true;
            }*/
        }
        else if(randNum==2) {

            arrow.setTexture(greenup);
            arrow.setSprite(sprite1);

            if(upFlag&&!lock) {
                arrow.setTexture(redup);
                score +=10;
                upFlag=false;
                lock = true;
            }
            // if press any other button, then lock it
            /*
            if(rightFlag||leftFlag||downFlag) {
                lock = true;
            }*/
        }
        else if(randNum==3) {
            arrow.setTexture(greendown);
            arrow.setSprite(sprite1);

            if(downFlag&&!lock) {
                arrow.setTexture(reddown);
                score +=10;
                downFlag = false;
                lock = true;
            }

            // if press any other button, then lock it
            /*if(rightFlag||upFlag||leftFlag) {
                lock = true;
            } */
        }
        else {
            // default to keep the current status
            // arrow keep on moving
        }

        if(lock) {
            randNum = 4;
            sleep(1);
        }
        // If a key is pressed
        if (event.type == sf::Event::KeyPressed)
        {
            switch (event.key.code)
            {
                // If escape is pressed, close the application
                case  sf::Keyboard::Escape : window.close(); break;

                // Process the up, down, left and right keys
                case sf::Keyboard::W :     {
                    upFlag=true;
                    downFlag=false;
                    leftFlag=false;
                    rightFlag=false;
                    break;
                }
                case sf::Keyboard::Z:    {
                    upFlag=false;
                    downFlag=true;
                    leftFlag=false;
                    rightFlag=false;
                    break;
                }
                case sf::Keyboard::A:    {
                    upFlag=false;
                    downFlag=false;
                    leftFlag=true;
                    rightFlag=false;
                    break;
                }
                case sf::Keyboard::D:   {
                    upFlag=false;
                    downFlag=false;
                    leftFlag=false;
                    rightFlag=true;
                    break;
                }
                default : break;

            }
            // test it works or not
            // vibrate(sf::milliseconds(10));
        }

        // If a key is released
        if (event.type == sf::Event::KeyReleased)
        {

            switch (event.key.code)
            {
                // Process the up, down, left and right keys
                case sf::Keyboard::W :     upFlag=false; break;
                case sf::Keyboard::Z:    downFlag=false; break;
                case sf::Keyboard::A:    leftFlag=false; break;
                case sf::Keyboard::D:   rightFlag=false; break;
                default : break;
            }

        }
        if(event.type == sf::Event::LostFocus)
            background = sf::Color::White;

        if(event.type== sf::Event::GainedFocus)
            background = sf::Color::Black;

        if(event.type == sf::Event::TouchBegan) {
            sf::Vector2f pos(event.touch.x, event.touch.y);
            isTouched(spr_Button, pos, bscale, leftFlag, rightFlag, upFlag, downFlag);
        }
        if (event.type == sf::Event::TouchEnded) //Mouse button Released now.
        {
            upFlag=false;
            downFlag=false;
            leftFlag=false;
            rightFlag=false;; //unlock when the button has been released.
        } //Released Scope

        if(gscore<score) {
            gscore = score;
        }
        window.clear();
        arrow.update();
        arrow.show(window);
        renderScore(font, score,gscore, width, window);
        //sf::sleep(sf::seconds(ranker));
        // std::cout << elapsed1.asSeconds() << std::endl;
        //clock.restart();
        window.draw(spr_Button);
        window.display();
    }

    writeScore2File(gfile, gscore);

    return 0;
}




int main2(int argc, char *argv[])
{
    sf::VideoMode screen(sf::VideoMode::getDesktopMode());

    sf::RenderWindow window(screen, "");
    window.setFramerateLimit(30);

    sf::Texture texture;
    if(!texture.loadFromFile("image.png"))
        return EXIT_FAILURE;

    sf::Sprite image(texture);
    image.setPosition(screen.width / 2, screen.height / 2);
    image.setOrigin(texture.getSize().x/2, texture.getSize().y/2);

    sf::Font font;
    if (!font.loadFromFile("sansation.ttf"))
        return EXIT_FAILURE;

    sf::Text text("Tap anywhere to move the logo.", font, 64);
    text.setFillColor(sf::Color::Black);
    text.setPosition(10, 10);

    // Loading canary.wav fails for me for now; haven't had time to test why

    /*sf::Music music;
    if(!music.openFromFile("canary.wav"))
        return EXIT_FAILURE;

    music.play();*/

    sf::View view = window.getDefaultView();

    sf::Color background = sf::Color::White;

    // We shouldn't try drawing to the screen while in background
    // so we'll have to track that. You can do minor background
    // work, but keep battery life in mind.
    bool active = true;

    while (window.isOpen())
    {
        sf::Event event;

        while (active ? window.pollEvent(event) : window.waitEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    if (event.key.code == sf::Keyboard::Escape)
                        window.close();
                    break;
                case sf::Event::Resized:
                    view.setSize(event.size.width, event.size.height);
                    view.setCenter(event.size.width/2, event.size.height/2);
                    window.setView(view);
                    break;
                case sf::Event::LostFocus:
                    background = sf::Color::Black;
                    break;
                case sf::Event::GainedFocus:
                    background = sf::Color::White;
                    break;

                // On Android MouseLeft/MouseEntered are (for now) triggered,
                // whenever the app loses or gains focus.
                case sf::Event::MouseLeft:
                    active = false;
                    break;
                case sf::Event::MouseEntered:
                    active = true;
                    break;
                case sf::Event::TouchBegan:
                    if (event.touch.finger == 0)
                    {
                        image.setPosition(event.touch.x, event.touch.y);
#if defined(USE_JNI)
                        vibrate(sf::milliseconds(10));
#endif
                    }
                    break;
            }
        }

        if (active)
        {
            window.clear(background);
            window.draw(image);
            window.draw(text);
            window.display();
        }
        else {
            sf::sleep(sf::milliseconds(100));
        }
    }
}
