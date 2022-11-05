#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include "Collision.hpp"
#include "windows.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include<vector>

using namespace std::chrono;
using namespace sf;
using namespace std;

//collision code
namespace Collision
{
class BitmaskManager
{
public:
    ~BitmaskManager()
    {
        std::map<const sf::Texture*, sf::Uint8*>::const_iterator end = Bitmasks.end();
        for (std::map<const sf::Texture*, sf::Uint8*>::const_iterator iter = Bitmasks.begin(); iter!=end; iter++)
            delete [] iter->second;
    }

    sf::Uint8 GetPixel (const sf::Uint8* mask, const sf::Texture* tex, unsigned int x, unsigned int y)
    {
        if (x>tex->getSize().x||y>tex->getSize().y)
            return 0;

        return mask[x+y*tex->getSize().x];
    }

    sf::Uint8* GetMask (const sf::Texture* tex)
    {
        sf::Uint8* mask;
        std::map<const sf::Texture*, sf::Uint8*>::iterator pair = Bitmasks.find(tex);
        if (pair==Bitmasks.end())
        {
            sf::Image img = tex->copyToImage();
            mask = CreateMask (tex, img);
        }
        else
            mask = pair->second;

        return mask;
    }

    sf::Uint8* CreateMask (const sf::Texture* tex, const sf::Image& img)
    {
        sf::Uint8* mask = new sf::Uint8[tex->getSize().y*tex->getSize().x];

        for (unsigned int y = 0; y<tex->getSize().y; y++)
        {
            for (unsigned int x = 0; x<tex->getSize().x; x++)
                mask[x+y*tex->getSize().x] = img.getPixel(x,y).a;
        }

        Bitmasks.insert(std::pair<const sf::Texture*, sf::Uint8*>(tex,mask));

        return mask;
    }
private:
    std::map<const sf::Texture*, sf::Uint8*> Bitmasks;
};

BitmaskManager Bitmasks;

bool PixelPerfectTest(const sf::Sprite& Object1, const sf::Sprite& Object2, sf::Uint8 AlphaLimit)
{
    sf::FloatRect Intersection;
    if (Object1.getGlobalBounds().intersects(Object2.getGlobalBounds(), Intersection))
    {
        sf::IntRect O1SubRect = Object1.getTextureRect();
        sf::IntRect O2SubRect = Object2.getTextureRect();

        sf::Uint8* mask1 = Bitmasks.GetMask(Object1.getTexture());
        sf::Uint8* mask2 = Bitmasks.GetMask(Object2.getTexture());

        // Loop through our pixels
        for (int i = Intersection.left; i < Intersection.left+Intersection.width; i++)
        {
            for (int j = Intersection.top; j < Intersection.top+Intersection.height; j++)
            {

                sf::Vector2f o1v = Object1.getInverseTransform().transformPoint(i, j);
                sf::Vector2f o2v = Object2.getInverseTransform().transformPoint(i, j);

                // Make sure pixels fall within the sprite's subrect
                if (o1v.x > 0 && o1v.y > 0 && o2v.x > 0 && o2v.y > 0 &&
                        o1v.x < O1SubRect.width && o1v.y < O1SubRect.height &&
                        o2v.x < O2SubRect.width && o2v.y < O2SubRect.height)
                {

                    if (Bitmasks.GetPixel(mask1, Object1.getTexture(), (int)(o1v.x)+O1SubRect.left, (int)(o1v.y)+O1SubRect.top) > AlphaLimit &&
                            Bitmasks.GetPixel(mask2, Object2.getTexture(), (int)(o2v.x)+O2SubRect.left, (int)(o2v.y)+O2SubRect.top) > AlphaLimit)
                        return true;

                }
            }
        }
    }
    return false;
}

bool CreateTextureAndBitmask(sf::Texture &LoadInto, const std::string& Filename)
{
    sf::Image img;
    if (!img.loadFromFile(Filename))
        return false;
    if (!LoadInto.loadFromImage(img))
        return false;

    Bitmasks.CreateMask(&LoadInto, img);
    return true;
}

sf::Vector2f GetSpriteCenter (const sf::Sprite& Object)
{
    sf::FloatRect AABB = Object.getGlobalBounds();
    return sf::Vector2f (AABB.left+AABB.width/2.f, AABB.top+AABB.height/2.f);
}

sf::Vector2f GetSpriteSize (const sf::Sprite& Object)
{
    sf::IntRect OriginalSize = Object.getTextureRect();
    sf::Vector2f Scale = Object.getScale();
    return sf::Vector2f (OriginalSize.width*Scale.x, OriginalSize.height*Scale.y);
}

bool CircleTest(const sf::Sprite& Object1, const sf::Sprite& Object2)
{
    sf::Vector2f Obj1Size = GetSpriteSize(Object1);
    sf::Vector2f Obj2Size = GetSpriteSize(Object2);
    float Radius1 = (Obj1Size.x + Obj1Size.y) / 4;
    float Radius2 = (Obj2Size.x + Obj2Size.y) / 4;

    sf::Vector2f Distance = GetSpriteCenter(Object1)-GetSpriteCenter(Object2);

    return (Distance.x * Distance.x + Distance.y * Distance.y <= (Radius1 + Radius2) * (Radius1 + Radius2));
}

class OrientedBoundingBox // Used in the BoundingBoxTest
{
public:
    OrientedBoundingBox (const sf::Sprite& Object) // Calculate the four points of the OBB from a transformed (scaled, rotated...) sprite
    {
        sf::Transform trans = Object.getTransform();
        sf::IntRect local = Object.getTextureRect();
        Points[0] = trans.transformPoint(0.f, 0.f);
        Points[1] = trans.transformPoint(local.width, 0.f);
        Points[2] = trans.transformPoint(local.width, local.height);
        Points[3] = trans.transformPoint(0.f, local.height);
    }

    sf::Vector2f Points[4];

    void ProjectOntoAxis (const sf::Vector2f& Axis, float& Min, float& Max) // Project all four points of the OBB onto the given axis and return the dotproducts of the two outermost points
    {
        Min = (Points[0].x*Axis.x+Points[0].y*Axis.y);
        Max = Min;
        for (int j = 1; j<4; j++)
        {
            float Projection = (Points[j].x*Axis.x+Points[j].y*Axis.y);

            if (Projection<Min)
                Min=Projection;
            if (Projection>Max)
                Max=Projection;
        }
    }
};

bool BoundingBoxTest(const sf::Sprite& Object1, const sf::Sprite& Object2)
{
    OrientedBoundingBox OBB1 (Object1);
    OrientedBoundingBox OBB2 (Object2);

    // Create the four distinct axes that are perpendicular to the edges of the two rectangles
    sf::Vector2f Axes[4] =
    {
        sf::Vector2f (OBB1.Points[1].x-OBB1.Points[0].x,
                      OBB1.Points[1].y-OBB1.Points[0].y),
        sf::Vector2f (OBB1.Points[1].x-OBB1.Points[2].x,
                      OBB1.Points[1].y-OBB1.Points[2].y),
        sf::Vector2f (OBB2.Points[0].x-OBB2.Points[3].x,
                      OBB2.Points[0].y-OBB2.Points[3].y),
        sf::Vector2f (OBB2.Points[0].x-OBB2.Points[1].x,
                      OBB2.Points[0].y-OBB2.Points[1].y)
    };

    for (int i = 0; i<4; i++) // For each axis...
    {
        float MinOBB1, MaxOBB1, MinOBB2, MaxOBB2;

        // ... project the points of both OBBs onto the axis ...
        OBB1.ProjectOntoAxis(Axes[i], MinOBB1, MaxOBB1);
        OBB2.ProjectOntoAxis(Axes[i], MinOBB2, MaxOBB2);

        // ... and check whether the outermost projected points of both OBBs overlap.
        // If this is not the case, the Separating Axis Theorem states that there can be no collision between the rectangles
        if (!((MinOBB2<=MaxOBB1)&&(MaxOBB2>=MinOBB1)))
            return false;
    }
    return true;
}
}



int random(int min, int max) //range : [min, max]
{
    static bool first = true;
    if (first)
    {
        srand( time(NULL) ); //seeding for the first time only!
        first = false;
    }
    return min + rand() % (( max + 1 ) - min);
}

int main()
{
    Clock clock = Clock();
    ContextSettings settings;
    settings.antialiasingLevel = 8;

    double pi=3.14;
    bool left = false, right = false,up=false,damage=false,play=true;
    int evilBallXPosition = 0,fuel=250,heart = 5,maxRotation = 50, roc_width,roc_height,roc_x,roc_y,fire_x,fire_y, width_dif;
    float speed = 1,speedX = 0,gravity = 0.01,levelGrav=0.01,angleRot=0;
    int time=0,counter=0;

    RenderWindow window(VideoMode(1280, 720), "Rocket Game!", Style::Default, settings);
    window.setFramerateLimit(30);

    // land
    Texture land,fireTex,expTex;
    if (!land.loadFromFile("images/land.png"))
        return EXIT_FAILURE;
    if(!fireTex.loadFromFile("images/fire.png")) return -1;
    if(!expTex.loadFromFile("images/explosion.png")) return -1;


    Texture rocketTex,landTex,bgTex;
    if(!bgTex.loadFromFile("images/space.png")) return -1;

    Sprite player, bgSp;
    bgSp.setTexture(bgTex);

    Texture playerTexture,studentTexture,evilTexture;


    if(!playerTexture.loadFromFile("images/rocket.png"))return -1;
    Sprite student,evilPlayer;

    player.setTexture(playerTexture);
    player.setScale(.125, .125) ;
    player.setOrigin(player.getGlobalBounds().width*4, player.getGlobalBounds().height*4);
    player.setPosition(window.getSize().x/2, window.getSize().y/2);


    Sprite sLand(land),fireSp,   expsp(expTex);
    sLand.setPosition(0, window.getSize().y - sLand.getGlobalBounds().height);


    fireSp.setTexture(fireTex);
    fireSp.setScale(.25, .25) ;
    fireSp.setOrigin(0,0) ;


    SoundBuffer expBuffer, bgBuffer,fireBuffer;
    if (!expBuffer.loadFromFile("audio/break.wav"))
        return -1;
    if(!bgBuffer.loadFromFile("audio/bg.wav"));
    if (!fireBuffer.loadFromFile("audio/fire.wav"))
        return -1;
    Sound expSound, bgSound, fireSound;

    fireSound.setBuffer(fireBuffer);
    expSound.setBuffer(expBuffer);
    bgSound.setBuffer(bgBuffer);
    bgSound.play();

    Font font;
    font.loadFromFile("Roboto.ttf");


    Text speedtext,fueltext,angletext,textGameOver,textwin,playtext,timeText;
    speedtext.setFont(font);
    speedtext.setCharacterSize(30);
    speedtext.setFillColor(sf::Color::Red);
    speedtext.setPosition(10,10);


    fueltext.setFont(font);
    fueltext.setCharacterSize(30);
    fueltext.setFillColor(sf::Color::Red);
    fueltext.setPosition(1100,10);


    angletext.setFont(font);
    angletext.setCharacterSize(30);
    angletext.setFillColor(sf::Color::Red);
    angletext.setPosition(900,50);

    textGameOver.setFont(font);
    textGameOver.setCharacterSize(100);
    textGameOver.setFillColor(sf::Color::Red);
    textGameOver.setString("game over! ");
    textGameOver.setOrigin(textGameOver.getGlobalBounds().width/2, textGameOver.getGlobalBounds().height/2);
    textGameOver.setPosition(window.getSize().x/2,-80+window.getSize().y/2);

    textwin.setFont(font);
    textwin.setCharacterSize(100);
    textwin.setFillColor(sf::Color::Blue);
    textwin.setString("congratulations! ");
    textwin.setOrigin(textwin.getGlobalBounds().width/2, textwin.getGlobalBounds().height/2);
    textwin.setPosition(window.getSize().x/2,window.getSize().y/2);


    playtext.setFont(font);
    playtext.setCharacterSize(100);
    playtext.setFillColor(sf::Color::White);
    playtext.setString("press (R) to play again ");
    playtext.setOrigin(playtext.getGlobalBounds().width/2, playtext.getGlobalBounds().height/2);
    playtext.setPosition(window.getSize().x/2,window.getSize().y/2+50);

    timeText.setFont(font);
    timeText.setCharacterSize(30);
    timeText.setFillColor(sf::Color::Red);
    timeText.setPosition(800,15);



    //user object
    int size = 50;




    while (window.isOpen())
    {
        Time ElapsedTime = clock.restart();

        counter+=1;
        time += counter%60;


        Event event;

        //game loop
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();


            //movement
            if (event.type == Event::KeyPressed)
            {
                if(event.key.code == Keyboard::Left)
                {
                    left = true;
                }
                if(event.key.code == Keyboard::Right)
                {
                    right = true;
                }


                if(event.key.code == Keyboard::A)
                {

                    up = true;
                    levelGrav=0.001;
                    fuel-=1;
                    if(fuel<=0){
                            fuel=0;
                        levelGrav=0;
                    }
                    fireSp.setScale(0.25,0.25);
                }
                if(event.key.code == Keyboard::S)
                {
                    up = true;
                    levelGrav=0.002;
                    fuel-=2;
                    if(fuel<=0){
                            fuel=0;
                        levelGrav=0;
                    }
                    fireSp.setScale(0.25,0.25);

                }
                if(event.key.code == Keyboard::D)
                {
                    up = true;
                    levelGrav=0.0022;
                    fuel-=3;
                    if(fuel<=0){
                            fuel=0;
                        levelGrav=0;
                    }
                    fireSp.setScale(0.25,0.25);

                }
                if(event.key.code == Keyboard::F)
                {
                    up = true;
                    levelGrav=0.009;
                    fuel-=5;
                    if(fuel<=0){
                            fuel=0;
                        levelGrav=0;
                    }
                    fireSp.setScale(0.3,0.3);

                    fireSound.play();
                }

                if(!play && event.key.code == Keyboard::R)
                {
                    play=true;
                    angleRot = player.getRotation();
                    gravity=levelGrav;
                    player.setPosition(640, 50);
                    time=0;
                    fuel= 250;

                }
            }
            if (event.type == Event::KeyReleased)
            {
                if(event.key.code == Keyboard::Left)
                {
                    left = false;
                }
                if(event.key.code == Keyboard::Up)
                {
                    up = false;
                }
                if(event.key.code == Keyboard::Right)
                {
                    right = false;
                }

                if(event.key.code == Keyboard::A)
                {
                    up = false;
                }
                if(event.key.code == Keyboard::S)
                {
                    up = false;
                }
                if(event.key.code == Keyboard::D)
                {
                    up = false;
                }

                if(event.key.code == Keyboard::F)
                {
                    up = false;
                }

            }
        }

        //logic
        if(play)
        {
            float x = 0,y = 0;


            if ((player.getPosition().y)<-20&&(player.getPosition().y>=-110))
            {
                play=false;
                damage=true;
            }


            if ((player.getPosition().y)<661.12&&(player.getPosition().y>=49.2))
            {
                gravity += 0.002;
            }

            y+=gravity;
            x = speedX;

            if (left)
            {
                if(player.getRotation() > 360 - maxRotation +5 || player.getRotation() < maxRotation)
                {
                    player.rotate(-3);
                    //angleRot+=-3;
                    fireSp.rotate(-3);
                }
            }
            if (right)
            {
                if(player.getRotation() > 360 - maxRotation || player.getRotation() < maxRotation -5)
                {
                    player.rotate(3);
                    fireSp.rotate(3);
                    // angleRot+=3;
                }
            }
            if (up)
            {
                if ((player.getPosition().y)<= 670.799988&&(player.getPosition().y>58.8))
                {
                    gravity -= levelGrav;
                }

                if(player.getRotation() > 300)
                {
                    speedX +=  -0.01*cos((360 - player.getRotation())*(pi/180));
                }
                else if (player.getRotation() > 0)
                {
                    speedX +=  0.01*cos(player.getRotation()*(pi/180));
                }
            }
            //

            roc_x = player.getPosition().x;
            roc_y = player.getPosition().y;
            roc_width= player.getGlobalBounds().width;
            roc_height= player.getGlobalBounds().height;

            width_dif = roc_width-fireSp.getGlobalBounds().width;

            fireSp.setPosition(roc_x + width_dif/2.0 -cos(player.getRotation()*(pi/180)) - fireSp.getGlobalBounds().width/2.0, roc_y+roc_height/2.0  );

            fire_x = fireSp.getPosition().x;
            fire_y = fireSp.getPosition().y;


            x = x*ElapsedTime.asMilliseconds();
            y = y *ElapsedTime.asMilliseconds();

            //borders
            //left
            if(player.getPosition().x - player.getGlobalBounds().width/2 + x  <= 0)
            {
                x = -(player.getPosition().x - player.getGlobalBounds().width/2);
            }
            //right
            if(player.getPosition().x + player.getGlobalBounds().width/2 + x  >= window.getSize().x)
            {
                x = window.getSize().x - player.getPosition().x - player.getGlobalBounds().width/2;
            }
            //up
            if(player.getPosition().y - player.getGlobalBounds().height/2+ y  <= -150)
            {
                y = -(player.getPosition().y - player.getGlobalBounds().height/2);
            }
            //down
            if(player.getPosition().y + player.getGlobalBounds().height/2 + y  >=  window.getSize().y)
            {
                y =  window.getSize().y - player.getPosition().y - player.getGlobalBounds().height/2;
            }
            player.move(x, y);


            if ( Collision::PixelPerfectTest( player, sLand ) )
            {

                if((roc_x > 650 && (roc_x+roc_width) <780))
                {
                    //cout<<"safe\n";

                    if(player.getRotation() == 0&&gravity<=.2)
                    {
                        //cout<<"win "<<player.getRotation()<<endl;
                        //  player.setPosition(window.getSize().x/2, window.getSize().y/2);
                        gravity=0;
                        play=false;
                        damage=false;
                    }
                    else
                    {
                        damage=true;
                        play=false;
                        expSound.play();
                    }
                }
                else
                {
                    damage=true;
                    expSound.play();
                    play=false;
                    //   player.setPosition(window.getSize().x/2, window.getSize().y/2);

                }
            }

        }

        //<--draw here-->
        if(damage && !play)
        {
            expsp.setPosition(player.getPosition().x,player.getPosition().y);

            window.draw(expsp);
            //cout<<player.getRotation()<<"dsf"<<endl;
            player.rotate(-player.getRotation());
            fireSp.rotate(-player.getRotation());
            //Sleep(10000);
            window.clear(Color::White);
            window.draw(bgSp);
            window.draw(textGameOver);
            window.draw(playtext);
        }

        else if(!damage&&!play)
        {
            window.clear(Color::White);
            window.draw(bgSp);
            window.draw(textwin);
        }

        else
        {
            window.clear();
            if(roc_x>650  && roc_x<770)
            {
                speedtext.setFillColor(Color::Green);
            }
            else
            {
                speedtext.setFillColor(Color::White);
            }
            //auto duration = duration_cast<microseconds>(stop - start);

            speedtext.setString("Speed "+to_string((int)(gravity*100)) + '\n' +"positionY: "+ to_string(roc_y)  + '\n' + "SpeedX: "+ to_string((int)(speedX*100)) + '\n' +"positionX: "+ to_string(roc_x) +'\n'+ "Time " + to_string(time/1000));


            window.draw(bgSp);

            window.draw(player);

            //fif(up)
               // window.draw(fireSp);

            window.draw(sLand);
            if((int)player.getRotation()==0)
            {
                angletext.setFillColor(Color::Green);
            }
            else
            {
                angletext.setFillColor(Color::White);
            }
            fueltext.setString("Fuel: "+to_string(fuel));
            angletext.setString("Angle: "+ to_string((int)player.getRotation()));

            window.draw(speedtext);
            window.draw(fueltext);
            window.draw(angletext);
            window.draw(angletext);
        }

        //<--end draw-->
        window.display();
    }
    return 0;

}






