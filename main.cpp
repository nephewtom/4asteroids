#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <list>
#include <vector>

using namespace sf;

//const int W = 1920;
const int W = 2560;
const int H = 1080;
// const int W = 800;
// const int H = 600;


float DEGTORAD = 0.017453f;

bool fullScreen = false;

class Score
{
  void config(Text& t) {
    t.setFont(font);
    t.setCharacterSize(45);
  }
  std::string makeString(int score, int bullets) {
    std::string s = std::to_string(score)+" / "+std::to_string(bullets);
    return s;
  }

public:
  Score(Font f) { 
    font = f;
    config(blue);
    config(green);

    std::string s = makeString(0, 30);
    blue.setFillColor(Color::Blue);
    blue.setString(s);
    blue.setPosition(100, 100);

    green.setFillColor(Color::Green);
    green.setString(s);
    green.setPosition(W-220, 100);
  }
  Font font;
  Text blue;
  Text green;

  void updateBlue(int score, int bullets) {
    std::string s = makeString(score, bullets);
    blue.setString(s);
  }
  void updateGreen(int score, int bullets) {
    std::string s = makeString(score, bullets);
    green.setString(s);
  }
  void draw(RenderWindow& window) {
    window.draw(blue);
    window.draw(green);
  }  
};

class Animation
{
public:
  float frame, speed;
  Sprite sprite;
  std::vector<IntRect> frames;

  Animation() {}
  Animation(Texture& t, int x, int y, int w, int h, int count, float s)
  {
    frame = 0;
    speed = s;

    for (int i = 0; i < count; i++) {
      frames.push_back(IntRect(x + i * w, y, w, h));

      sprite.setTexture(t);
      sprite.setOrigin(w / 2, h / 2);
      sprite.setTextureRect(frames[0]);
    }
  }

  void update() {
    frame += speed;
    int n = frames.size();
    if (frame >= n) frame -= n;
    if (n > 0) sprite.setTextureRect(frames[int(frame)]);
  }

   bool isEnd() {
     return frame+speed>=frames.size();
   }  
};

class Entity
{
public:
  float x, y, dx, dy, r, angle;
  bool life;
  std::string name;
  Animation anim;
  Animation animQuiet;

  Entity() { life = 1; }
  void settings(Animation& a, int X, int Y, float Angle = 0, int radius = 1) {
    x = X; y = Y; anim = a; animQuiet = a;
    angle = Angle; r = radius;
  }

  virtual void update() {};

  virtual void draw(RenderWindow& window) {
    anim.sprite.setPosition(x, y);
    anim.sprite.setRotation(angle + 90);
    window.draw(anim.sprite);
  }
};

class Bullet : public  Entity
{
public:
  Bullet(std::string s) { name = s; }
  void update() {
    dx = cos(angle * DEGTORAD) * 30;
    dy = sin(angle * DEGTORAD) * 30;
    x += dx;
    y += dy;
    if (x > W || x<0 || y>H || y < 0) life = 0;
  }
};

class Asteroid : public Entity
{

};

class Player : public Entity
{
  const Joystick::Axis padAxisX = static_cast<Joystick::Axis>(0);
  const Joystick::Axis padAxisY = static_cast<Joystick::Axis>(1);
  const int padButtonA = 0;

  Animation animGo;
  Animation sBullet;
  std::list<Entity*>& entities;

  Sound laserSound;
  Sound rechargeSound;

  Clock clock;
  bool outOfBullets = false;

public:
  int jx = 0, jy = 0;
  bool buttonA = false;
  bool pressedButtonA = false;

  bool thrust = false;
  int bullets;
  int score = 0;

  Player(std::string s, Animation& ag, Animation& a, Sound& snd1, Sound& snd2, std::list<Entity*>& e) : entities(e) {
    name = s;
    animGo = ag;
    sBullet = a;
    laserSound = snd1;
    rechargeSound = snd2;
    bullets = 30;
    score = 0;
  }

  void update()
  {
    if (jx >= 40.0) angle += 3;
    else if (jx <= -40) angle -= 3;

    if (thrust) {
      dx += cos(angle * DEGTORAD) * 0.2;
      dy += sin(angle * DEGTORAD) * 0.2;
      anim = animGo;
    }
    else {
      dx *= 0.99;
      dy *= 0.99;
      anim = animQuiet;
    }

    int maxSpeed = 15;
    float speed = sqrt(dx * dx + dy * dy);
    if (speed > maxSpeed) {
      dx *= maxSpeed / speed;
      dy *= maxSpeed / speed;
    }

    x += dx;
    y += dy;

    if (x > W) x = 0; if (x < 0) x = W; 
    if (y > H) y = 0; if (y < 0) y = H;

    if (pressedButtonA && buttonA == false) {
      if (bullets > 0) {
        bullets--;
        std::string bname = "bullet" + name;
        Bullet* b = new Bullet(bname);
        b->settings(sBullet, x, y, angle, 10);
        entities.push_back(b);
        laserSound.play();
      } else {
        rechargeSound.play();
        if (outOfBullets) {
          float elapsed = clock.getElapsedTime().asSeconds();
          if (outOfBullets && elapsed > 7.0f) {
            bullets = 30;
            outOfBullets = false;
          }
        } else {
          clock.restart();
          outOfBullets = true;
        }
      }

      pressedButtonA = false;
    }
  }

  void joystick(int id) {

    jx = Joystick::getAxisPosition(id, padAxisX);
    jy = Joystick::getAxisPosition(id, padAxisY);

    if (jy >= -20) thrust = false;
    if (jy < -20) thrust = true;

    if (Joystick::isButtonPressed(id, padButtonA)) {
      buttonA = true;
      pressedButtonA = true;
    } else {
      buttonA = false;
    }
  }
};

bool isCollide(Entity* a, Entity* b)
{
  return (b->x - a->x) * (b->x - a->x) +
    (b->y - a->y) * (b->y - a->y) <
    (a->r + b->r) * (a->r + b->r);
}

int main()
{
  srand(time(0));

  for (int i=0; i<100; i++) {
    printf("%i ", rand()%W);
  }
  for (int i=0; i<100; i++) {
    printf("%i ", rand()%H);
  }

  Music music;
  std::string s("sounds/spacemusic1.ogg");
  //if (!music.openFromFile("sounds/spacemusic1.ogg"))
  if (!music.openFromFile(s))
    return 1;
  music.setLoop(true);
  music.play();

  Font scoreFont;
  if (!scoreFont.loadFromFile("fonts/orbitron/Orbitron Black.otf")) {
    return 1;
  }

  SoundBuffer bufferBlue, bufferGreen, bufferExplosion, bufferRecharge;
  if (!bufferBlue.loadFromFile("sounds/laser2.wav"))
      return 1;
  if (!bufferGreen.loadFromFile("sounds/laser1.wav"))
      return 1;
  if (!bufferExplosion.loadFromFile("sounds/explosion1.wav"))
      return 1;
  if (!bufferRecharge.loadFromFile("sounds/recharge.wav"))
      return 1;

  Sound laserSoundBlue(bufferBlue);
  Sound laserSoundGreen(bufferGreen);
  Sound explosionSound(bufferExplosion);
  Sound rechargeSound(bufferRecharge);

  RenderWindow window(VideoMode(W, H), "Asteroids!",  Style::Fullscreen);// Style::Resize);//,
  window.setFramerateLimit(60);

  Texture tBackground;
  tBackground.loadFromFile("images/stars2.jpg");
  tBackground.setSmooth(true);
  Sprite sBackground(tBackground);

  Texture tExplosionShip;
  tExplosionShip.loadFromFile("images/explosions/type_B.png");
  Animation sExplosionShip(tExplosionShip, 0,0,192,192, 64, 0.5);

  Texture tPlayerBlue, tBulletBlue;
  tPlayerBlue.loadFromFile("images/blueship.png");
  tPlayerBlue.setSmooth(true);
  Animation sPlayerBlue(tPlayerBlue, 40, 0, 40, 40, 1, 0);
  Animation sPlayerBlueGo(tPlayerBlue, 40,40,40,40, 1, 0);
  tBulletBlue.loadFromFile("images/bulletBlue.png");
  Animation sBulletBlue(tBulletBlue, 0, 0, 32, 64, 16, 0.8);

  Texture tPlayerGreen, tBulletGreen;
  tPlayerGreen.loadFromFile("images/greenship.png");
  tPlayerGreen.setSmooth(true);
  Animation sPlayerGreen(tPlayerGreen, 40, 0, 40, 40, 1, 0);
  Animation sPlayerGreenGo(tPlayerGreen, 40, 40, 40, 40, 1, 0);
  tBulletGreen.loadFromFile("images/bulletGreen.png");
  Animation sBulletGreen(tBulletGreen, 0, 0, 32, 64, 16, 0.8);

  std::list<Entity*> entities;

  Player* playerBlue = new Player("Blue", sPlayerBlueGo, sBulletBlue, laserSoundBlue, rechargeSound, entities);
  playerBlue->settings(sPlayerBlue, 20, H/2, 0, 20);
  entities.push_back(playerBlue);

  Player* playerGreen = new Player("Green", sPlayerGreenGo, sBulletGreen, laserSoundGreen, rechargeSound, entities);
  playerGreen->settings(sPlayerGreen, W-20, H/2, -180, 20);
  entities.push_back(playerGreen);

  Player* players[4];
  players[0] = playerBlue;
  players[1] = playerGreen;

  Score score(scoreFont);

  while (window.isOpen()) {
    Event e;
    while (window.pollEvent(e)) {
      if (e.type == Event::Closed) {
        window.close();
      }

      if (e.type == Event::KeyPressed) {

        if (e.key.code == Keyboard::Escape) exit(0);
      }

      else if ((e.type == Event::JoystickButtonPressed) ||
        (e.type == Event::JoystickButtonReleased) ||
        (e.type == Event::JoystickMoved) ||
        (e.type == Event::JoystickConnected)) {

        // Update displayed joystick values
        int id = e.joystickConnect.joystickId;
        players[id]->joystick(id);
        score.updateBlue(playerBlue->score, playerBlue->bullets);
        score.updateGreen(playerGreen->score, playerGreen->bullets);     
      }

      else if (e.type == Event::JoystickDisconnected)
      {
        ;
      }

    }

    for(auto e:entities)
     if (e->name=="explosion")
      if (e->anim.isEnd()) e->life=0;

    for (auto a : entities) {
      for (auto b : entities) {
        if (a->name == "Green" && b->name == "bulletBlue" ||
            a->name == "Blue" && b->name == "bulletGreen") {

          if (isCollide(a,b)) {
            explosionSound.play();
            Entity* e = new Entity();
            e->settings(sExplosionShip, a->x, a->y);
            e->name = "explosion";
            entities.push_back(e);

            a->settings(a->animQuiet, rand()%W, rand()%H, 0, 20);
            a->dx = 0; a->dy = 0;
            ((Player*)a)->score--;
            score.updateBlue(playerBlue->score, playerBlue->bullets);
            score.updateGreen(playerGreen->score, playerGreen->bullets);          
          }
        }
        if (a->name == "Green" && b->name == "Blue") {
          if (isCollide(a,b)) {
            explosionSound.play();
            Entity* e = new Entity();
            e->settings(sExplosionShip, a->x, a->y);
            e->name = "explosion";
            entities.push_back(e);

            explosionSound.play();
            e = new Entity();
            e->settings(sExplosionShip, b->x, b->y);
            e->name = "explosion";
            entities.push_back(e);

            a->settings(a->animQuiet, rand()%W, rand()%H, 0, 20);
            a->dx = 0; a->dy = 0;
            ((Player*)a)->score--;
            b->settings(b->animQuiet, rand()%W, rand()%H, 0, 20);
            b->dx = 0; b->dy = 0;
            ((Player*)b)->score--;            
            score.updateBlue(playerBlue->score, playerBlue->bullets);
            score.updateGreen(playerGreen->score, playerGreen->bullets);                 
          }
        }
      }
    }

    for (auto i = entities.begin(); i != entities.end();) {
      Entity* e = *i;
      e->update();
      e->anim.update();

      if (e->life == false) { i = entities.erase(i); delete e; }
      else i++;
    }

    // draw
    window.clear();
    window.draw(sBackground);

    for (auto e : entities) {
      e->draw(window);
    }

    score.draw(window);
    window.display();
  }

  return 0;
}
