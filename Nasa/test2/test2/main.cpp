#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include "Collision.hpp"

using namespace sf;
using namespace std;

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
    RenderWindow window(VideoMode(1280, 720), "SFML works!", Style::Default, settings);
    window.setFramerateLimit(60);
    // land
    Texture land;
        if (!land.loadFromFile("/Users/ahmedhany/projects/test app/test2/test2/images/land.png"))
            return EXIT_FAILURE;
        Sprite sLand(land);
        sLand.setPosition(0, window.getSize().y - sLand.getGlobalBounds().height +200);
    
    //sound
    Music music;
    if (!music.openFromFile("/Users/ahmedhany/projects/test app/test2/test2/cann.wav"))
        return -1;
    music.play();
    
    SoundBuffer buffer;
    if (!buffer.loadFromFile("/Users/ahmedhany/projects/test app/test2/test2/jump.wav"))
        return -1;
    Sound sound;
    sound.setBuffer(buffer);
    
    
    //textScore
    
    Font font;
    font.loadFromFile("/Users/ahmedhany/projects/test app/test2/test2/Roboto.ttf");
    
    
    
    Texture rocketTex,landTex,bgTex;
    if(!bgTex.loadFromFile("/Users/ahmedhany/projects/test app/test2/test2/images/bg.jpeg")) return -1;

   Sprite rocketSp, bgSp;
    bgSp.setTexture(bgTex);
    
   
   

   Text speedtext,fueltext,angletext;
   speedtext.setFont(font);
   speedtext.setCharacterSize(30);
   speedtext.setFillColor(sf::Color::White);
   speedtext.setPosition(10,10);
   



   fueltext.setFont(font);
   fueltext.setCharacterSize(30);
   fueltext.setFillColor(sf::Color::White);
   fueltext.setPosition(1100,10);
    fueltext.setString("Fuel: ");



   angletext.setFont(font);
   angletext.setCharacterSize(30);
   angletext.setFillColor(sf::Color::White);
   angletext.setPosition(900,50);
        
    
    //user object
    int size = 50;
    Texture playerTexture,studentTexture,evilTexture;

    
   if(!playerTexture.loadFromFile("/Users/ahmedhany/projects/test app/test2/test2/images/rocket.png")) return -1;
    Sprite player,student,evilPlayer;
    
    player.setTexture(playerTexture);
    player.setScale(.25, .25) ;
    player.setOrigin(player.getGlobalBounds().width*2, player.getGlobalBounds().height*2);
    player.setPosition(window.getSize().x/2, window.getSize().y/2);
    
    
    //student object
    studentTexture.loadFromFile("/Users/ahmedhany/projects/test app/test2/test2/images/student.png");
    student.setTexture(studentTexture);
    student.setPosition(random(size*2,window.getSize().x -size*2), random(size*2,window.getSize().y -size*2));
    


    
    bool left = false, right = false,up=false ;
    int evilBallXPosition = 0,score=0,heart = 5,maxRotation = 45 +5;
    float speed = 1,speedX = 0,gravity = 0.04;
    int fuel;
    while (window.isOpen())
    {
        Time ElapsedTime = clock.restart();
        
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();
            
            //movment
            if (event.type == Event::KeyPressed){
                if(event.key.code == Keyboard::Left){
                    left = true;
                }
                 if(event.key.code == Keyboard::Right){
                     right = true;
                }
                if(event.key.code == Keyboard::Up){
                    up = true;
                }
            }
            if (event.type == Event::KeyReleased){
                if(event.key.code == Keyboard::Left){
                    left = false;
                }
                if(event.key.code == Keyboard::Up){
                    up = false;
                }
                 if(event.key.code == Keyboard::Right){
                     right = false;
                }
            }
        }
        
        
        
        //logic
        
        float x = 0,y = 0;
        
        gravity += 0.002;
        y+=gravity;
        x = speedX;
        
        if (left){
            cout << player.getRotation();
            if(player.getRotation() > 360 - maxRotation +5 || player.getRotation() < maxRotation){
                player.rotate(-5);
            }
        }
        if (right){
            if(player.getRotation() > 360 - maxRotation || player.getRotation() < maxRotation -5){
                player.rotate(5);
            }
        }
        if (up){
            gravity -= 0.01;
            if(player.getRotation() > 300){
                speedX +=  -0.01*cos(360 - player.getRotation());
            } else if (player.getRotation() > 0) {
                speedX +=  0.01*cos(player.getRotation());
            }
            
        }
        
        if (x && y) {
            x = x/1.5;
            y= y/1.5;
        }
        x = x*ElapsedTime.asMilliseconds();
        y = y *ElapsedTime.asMilliseconds();
        //left
        if(player.getPosition().x - player.getGlobalBounds().width/2 + x  <= 0){
            x = -(player.getPosition().x - player.getGlobalBounds().width/2);
        }
        //right
        if(player.getPosition().x + player.getGlobalBounds().width/2 + x  >= window.getSize().x){
            x = window.getSize().x - player.getPosition().x - player.getGlobalBounds().width/2;
        }
        //up
        if(player.getPosition().y - player.getGlobalBounds().height/2 + y  <= 0){
            y = -(player.getPosition().y - player.getGlobalBounds().height/2);
        }
        //down
        if(player.getPosition().y + player.getGlobalBounds().height/2 + y  >=  window.getSize().y){
            y =  window.getSize().y - player.getPosition().y - player.getGlobalBounds().height/2;
        }
        player.move(x, y);
        
        if ( Collision::PixelPerfectTest( player, student ) )
        {
             Vector2f newPos = Vector2f(random(size*2, window.getSize().x -size*2), random(size*2, window.getSize().y -size*2));
             student.setPosition(newPos);

             score++;
             sound.play();
        }
        
        
        
        if(player.getGlobalBounds().intersects(evilPlayer.getGlobalBounds())){
            evilPlayer.setPosition(50,50);
            heart--;
            sound.play();
        }
        
        //<--draw here-->
        if(heart<=0){
            window.clear();
            
            window.draw(bgSp);

            window.draw(rocketSp);
            window.draw(speedtext);
            window.draw(fueltext);
            window.draw(angletext);
        } else{
            window.clear(Color::White);
            window.draw(bgSp);
            window.draw(sLand);
            
            
            window.draw(player);
            window.draw(student);
            window.draw(evilPlayer);
           

            window.draw(rocketSp);
            speedtext.setString("SpeedY: "+ to_string((int)(gravity*100)) + "positionY: "+ to_string(player.getPosition().y)  );
            window.draw(speedtext);
            window.draw(fueltext);
            
            
            angletext.setString("Angle: "+ to_string((int)player.getRotation()));
            window.draw(angletext);
        }
        
        //<--end draw-->
        window.display();
    }
    return 0;
}

