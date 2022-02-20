#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <list>
#include <vector>

using namespace sf;
using namespace std;

struct Pair {
  union {
    int x;
    int w;
  };
  union {
    int y;
    int h;
  };
};

Pair resolution = { 0, 0 };
const char* windowTitle = "Space4U";

float wFactor[3] = { 0.5f, 2 / 3.0f, 1.0f }; // screen factors
int wIndex = 0;

float DEGTORAD = 0.017453f;

bool isFullScreen = false;
void toggleFullScreen(RenderWindow& window) {

  VideoMode videoMode = VideoMode::getDesktopMode();
  printf("-----\n");
  printf("Current VideoMode: (%d, %d)\n", videoMode.width, videoMode.height);

  if (isFullScreen) {
    printf("Toggle to Window!\n");
    window.create(VideoMode(resolution.w, resolution.h), windowTitle);
  }
  else {
    printf("toggle to Fullscreen!\n");
    window.create(VideoMode(resolution.w, resolution.h), windowTitle, Style::Fullscreen);
  }
  isFullScreen = !isFullScreen;
  printf("\n");
}

class Animation
{
public:
  float frame, speed;
  Sprite sprite;
  std::vector<IntRect> frames;

  Animation() {}
  Animation(Texture& t, int x, int y, int w, int h, int count, float s) {
    set(t, x, y, w, h, count, s);
  }

  void set(Texture& t, int x, int y, int w, int h, int count, float s) {
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
    if (frame >= n) { frame -= n; }
    if (n > 0) { sprite.setTextureRect(frames[int(frame)]); }
  }

  bool isEnd() {
    return frame + speed >= frames.size();
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
    dx = dy = 0;
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
    dx = cos(angle * DEGTORAD) * 10;
    dy = sin(angle * DEGTORAD) * 10;
    x += dx;
    y += dy;
    if (x > resolution.w || x<0 || y>resolution.h || y < 0) life = 0;
  }
};

class Score
{
  void config(Text& t) {
    t.setFont(font);
    t.setCharacterSize(45);
  }
  std::string makeString(int score, int bullets) {
    std::string s = std::to_string(score) + " / " + std::to_string(bullets);
    return s;
  }

public:
  Score() {}

  void set(Font f) {
    font = f;
    config(blue);
    config(green);

    std::string s = makeString(0, 30);
    blue.setFillColor(Color::Blue);
    blue.setString(s);
    blue.setPosition(100, 100);

    green.setFillColor(Color::Green);
    green.setString(s);
    green.setPosition(resolution.w - 220, 100);
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

struct Global {
  
  //RenderWindow& window;

  Font font;
  Texture tBackground;
  Sprite sBackground;
  vector<VideoMode> availableVideoModes;
  int videoModeIndex;
  int numberOfPlayers;

  //Global(RenderWindow& w) : window(w) { }

  int init() {

    initVideoModes();

    if (!font.loadFromFile("fonts/orbitron/Orbitron Black.otf")) {
      return 1;
    }

    tBackground.loadFromFile("images/stars2.jpg");
    tBackground.setSmooth(true);
    sBackground.setTexture(tBackground);

    numberOfPlayers = 2;
    return 0;
  }

  void initVideoModes() {

    printf("Resolutions:\n");
    availableVideoModes = VideoMode::getFullscreenModes();
    for (int i = 0; i < availableVideoModes.size(); i++) {
      VideoMode mode = availableVideoModes[i];
      printf("(%i, %i)\n", mode.width, mode.height);
      if (mode.width == 1024 && mode.height == 768) {
        resolution.w = mode.width;
        resolution.h = mode.height;
        videoModeIndex = i;
      }
    }
    if (resolution.w == 0) {
      resolution.w = availableVideoModes[0].width;
      resolution.h = availableVideoModes[0].height;
      videoModeIndex = 0;
    }
  }

  void draw(RenderWindow& w) {
    w.draw(sBackground);
  }
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
  int jx, jy;
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
    jx = 0; jy = 0;
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

    if (x > resolution.w) x = 0; if (x < 0) x = resolution.w;
    if (y > resolution.h) y = 0; if (y < 0) y = resolution.h;

    if (pressedButtonA && buttonA == false) {
      if (bullets > 0) {
        bullets--;
        std::string bname = "bullet" + name;
        Bullet* b = new Bullet(bname);
        b->settings(sBullet, x, y, angle, 10);
        entities.push_back(b);
        laserSound.play();
      }
      else {
        rechargeSound.play();
        if (outOfBullets) {
          float elapsed = clock.getElapsedTime().asSeconds();
          if (outOfBullets && elapsed > 7.0f) {
            bullets = 30;
            outOfBullets = false;
          }
        }
        else {
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

    if (jy >= -20) { thrust = false; }
    else { thrust = true; }

    if (Joystick::isButtonPressed(id, padButtonA)) {
      buttonA = true;
      pressedButtonA = true;
    }
    else {
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

class Screen {
public:
  enum Action { Exit = -1, Ok = 0, Menu = 0, Game = 1, Keep = 2 };
  virtual int run(RenderWindow& window) = 0;
};

class MenuEntry {
public:
  Font font;
  Text text;
  MenuEntry* subMenu;
  const Color defaultColor = Color::Blue;

  MenuEntry(String name, Font& f, unsigned int size, MenuEntry* sub = 0) : font(f), subMenu(sub) {
    text.setFont(font);
    text.setString(name);
    text.setCharacterSize(size);
    text.setFillColor(defaultColor);
    text.setOutlineColor(Color::Green);
    text.setOutlineThickness(1.0f);
  }
  void setPosition(const Vector2f& v) {
    text.setPosition(v);
  }

  void setFillColor(Color color) {
    text.setFillColor(color);
  }
  void setString(String s) {
    text.setString(s);
  }
  void draw(RenderWindow& window) {
    window.draw(text);
    if (subMenu) {
      window.draw(subMenu->text);
    }
  }
};


class Menu : public Screen {
  Font& font;
  Global& global;

  Text title;

  vector<MenuEntry*> menus;
  enum MenuIndex { Play = 0, Resolution, Players, Exit };
  String playText = "Play";
  String resolutionText = "Resolution";
  String playersText = "Players";
  String exitText = "Exit";
  int currentMenu;

  bool inSubMenu;
  MenuEntry* menuResolution;
  MenuEntry* subMenuResolution;
  MenuEntry* menuPlayers;
  MenuEntry* subMenuPlayers;

  Text help;

public:
  static const Color highlightColor;
  static const Color defaultColor;
  Menu(Global& g) : global(g), font(g.font), inSubMenu(false) { }

  int init() {

    MenuEntry* mPlay = new MenuEntry(playText, font, 100 * wFactor[wIndex]);

    int& np = global.numberOfPlayers;
    subMenuPlayers = new MenuEntry((char)(np + 0x30), font, 100 * wFactor[wIndex]);
    menuPlayers = new MenuEntry(playersText, font, 100 * wFactor[wIndex], subMenuPlayers);

    auto vm = global.availableVideoModes[global.videoModeIndex];
    char buffer[16];
    sprintf(buffer, "%ix%i", vm.width, vm.height);
    subMenuResolution = new MenuEntry(buffer, font, 80 * wFactor[wIndex]);
    menuResolution = new MenuEntry(resolutionText, font, 100 * wFactor[wIndex], subMenuResolution);

    MenuEntry* mExit = new MenuEntry(exitText, font, 100 * wFactor[wIndex]);

    menus.push_back(mPlay);
    menus.push_back(menuResolution);
    menus.push_back(menuPlayers);
    menus.push_back(mExit);

    int n = 2;
    for (auto m : menus) {
      m->setPosition(Vector2f(resolution.w / 5.0f, n * resolution.h / 8.0f));
      if (m->subMenu) {
        m->subMenu->setPosition(Vector2f(2.8f * resolution.w / 5.0f, n * resolution.h / 8.0f));
      }
      n++;
    }

    help.setFont(font);
    help.setFillColor(Color::Yellow);
    help.setOutlineColor(Color::Red);
    help.setCharacterSize(50 * wFactor[wIndex]);
    help.setOutlineThickness(1.0f);

    title.setFont(font);
    title.setFillColor(Color::Magenta);
    title.setOutlineColor(Color::Green);
    title.setCharacterSize(150 * wFactor[wIndex]);
    title.setOutlineThickness(3.0f);
    title.setString("Space4U");
    FloatRect tRect = title.getLocalBounds();
    title.setOrigin(tRect.left + tRect.width / 2, tRect.top + tRect.height / 2);
    title.setPosition(Vector2f(resolution.w / 2.0f, 1 * resolution.h / 8.0f));


    currentMenu = Play;
    menus[currentMenu]->setFillColor(Color::Red);
    updateHelpMenu();
    return Screen::Ok;
  }

  void updateHelpMenu() {
    // String s = "ENTER or A button to ";
    String s = "";
    switch (currentMenu) {
    case Play:
      help.setString(s + "Play game");
      break;
    case Resolution:
      help.setString(s + "Change resolution");
      break;
    case Players:
      help.setString(s + "Set number of players (2-4)");
      break;
    case Exit:
      help.setString(s + "Exit");
      break;
    }
    FloatRect tRect = help.getLocalBounds();
    help.setOrigin(tRect.left + tRect.width / 2, tRect.top + tRect.height / 2);
    help.setPosition(Vector2f(resolution.w / 2.0f, 7 * resolution.h / 8.0f));
  }

  void handleUpEvent() {
    if (!inSubMenu) {
      menus[currentMenu]->setFillColor(defaultColor);
      currentMenu--;
      currentMenu = currentMenu < 0 ? menus.size() - 1 : currentMenu;
      menus[currentMenu]->setFillColor(highlightColor);

      updateHelpMenu();
      return;
    }
    if (currentMenu == Resolution) {

      auto& vm = global.availableVideoModes;
      auto& index = global.videoModeIndex;
      index--;
      index = index < 0 ? vm.size() - 1 : index;
      //printf("Up | index:%i\n", index);
      char buffer[16];
      sprintf(buffer, "%ix%i", vm[index].width, vm[index].height);
      subMenuResolution->setString(buffer);
    }
    if (currentMenu == Players) {
      int& np = global.numberOfPlayers;
      np--;
      np = np < 2 ? 4 : np;
      subMenuPlayers->setString((char)(np + 0x30));
    }

  }
  void handleDownEvent() {
    if (!inSubMenu) {
      menus[currentMenu]->setFillColor(defaultColor);
      currentMenu++;
      currentMenu = (currentMenu == menus.size()) ? 0 : currentMenu;
      menus[currentMenu]->setFillColor(highlightColor);

      updateHelpMenu();
      return;
    }
    if (currentMenu == Resolution) {
      auto& vm = global.availableVideoModes;
      auto& index = global.videoModeIndex;

      index++;
      index = index == vm.size() ? 0 : index;
      //printf("Down | index:%i\n", index);
      char buffer[16];
      sprintf(buffer, "%ix%i", vm[index].width, vm[index].height);
      subMenuResolution->setString(buffer);
    }
    if (currentMenu == Players) {
      int& np = global.numberOfPlayers;
      np++;
      np = np > 4 ? 2 : np;
      subMenuPlayers->setString((char)(np + 0x30));
    }
  }

  Screen::Action handleReturnEvent() {

    if (currentMenu == Play) {
      for (int i = 0; i < global.numberOfPlayers; i++) {
        if (!Joystick::isConnected(i)) {
          char buffer[128];
          sprintf(buffer, "Game can not start, joystick: %i not connected.\n", i);
          updateHelpWithWarning(buffer);
          return Screen::Keep;
        }
      }

      return Screen::Game;
    }

    if (currentMenu == Players) {
      if (!inSubMenu) {
        inSubMenu = true;
        menuPlayers->text.setColor(defaultColor);
        subMenuPlayers->text.setColor(highlightColor);
      } else {
        menuPlayers->text.setColor(highlightColor);
        subMenuPlayers->text.setColor(defaultColor);
        inSubMenu = false; 
      }
    }

    if (currentMenu == Resolution) {
      if (!inSubMenu) {
        inSubMenu = true;
        menuResolution->text.setColor(defaultColor);
        subMenuResolution->text.setColor(highlightColor);
      }
      else {
        menuResolution->text.setColor(highlightColor);
        subMenuResolution->text.setColor(defaultColor);
        inSubMenu = false;

        //global.window.create(VideoMode(resolution.w, resolution.h), windowTitle);
      }
      return Screen::Keep;
    }

    if (currentMenu == Exit) {
      return Screen::Exit;
    }

    return Screen::Keep;
  }

  Screen::Action handleEscapeEvent() {
    if (!inSubMenu) {
      // TODO: Remove when game finished
      return Screen::Exit;
    }

    if (currentMenu == Players) {
      menuPlayers->text.setColor(highlightColor);
      subMenuPlayers->text.setColor(defaultColor);
      inSubMenu = false;
    }
    if (currentMenu == Resolution) {
      menuResolution->text.setColor(highlightColor);
      subMenuResolution->text.setColor(defaultColor);
      inSubMenu = false;
    }
    return Screen::Keep;
  }

  void updateHelpWithWarning(const char* msg) {
    help.setString(msg);
    FloatRect tRect = help.getLocalBounds();
    help.setOrigin(tRect.left + tRect.width / 2, tRect.top + tRect.height / 2);
    help.setPosition(Vector2f(resolution.w / 2.0f, 7 * resolution.h / 8.0f));
  }

  void handleJoystick(int id) {

  }

  virtual int run(RenderWindow& window) {
    bool running = true;


    while (running) {

      Event event;
      while (window.pollEvent(event)) {
        if (event.type == Event::Closed)
          return -1;

        if (event.type == Event::KeyPressed) {
          if (event.key.code == Keyboard::Escape) {
            int action = handleEscapeEvent();
            if (action != Screen::Keep) {
              return action;
            }
          }

          if (event.key.code == Keyboard::Up) { handleUpEvent(); }
          if (event.key.code == Keyboard::Down) { handleDownEvent(); }
          if (event.key.code == Keyboard::Return) {
            int action = handleReturnEvent();
            if (action != Screen::Keep) {
              return action;
            }
          }

          if ((event.type == Event::JoystickButtonPressed) ||
            (event.type == Event::JoystickButtonReleased) ||
            (event.type == Event::JoystickMoved) ||
            (event.type == Event::JoystickConnected)) {

            int id = event.joystickConnect.joystickId;
            handleJoystick(id);
          }

          if (event.key.code == Keyboard::F11) {
            toggleFullScreen(window);
          }
        }
      }

      window.clear();
      global.draw(window);
      window.draw(title);
      for (auto m : menus) {
        m->draw(window);
      }
      window.draw(help);
      window.display();
    }

    return 0;
  }
};
const Color Menu::highlightColor = Color::Red;
const Color Menu::defaultColor = Color::Blue;

class Game : public Screen {
  Font& font;
  Global& global;

  SoundBuffer bufferBlue, bufferGreen, bufferExplosion, bufferRecharge;
  Sound laserSoundBlue, laserSoundGreen, explosionSound, rechargeSound;

  Texture tExplosionShip;
  Animation sExplosionShip;

  Texture tPlayerBlue, tBulletBlue;
  Animation sPlayerBlue, sPlayerBlueGo, sBulletBlue;

  Texture tPlayerGreen, tBulletGreen;
  Animation sPlayerGreen, sPlayerGreenGo, sBulletGreen;

  std::list<Entity*> entities;

  Player* playerBlue, * playerGreen;
  Player* players[4];

  Score score;

public:
  Game(Global& g) : global(g), font(g.font) { }

  int init() {

    if (!bufferBlue.loadFromFile("sounds/laser2.wav"))
      return 1;
    if (!bufferGreen.loadFromFile("sounds/laser1.wav"))
      return 1;
    if (!bufferExplosion.loadFromFile("sounds/explosion1.wav"))
      return 1;
    if (!bufferRecharge.loadFromFile("sounds/recharge.wav"))
      return 1;

    // Sounds
    laserSoundBlue.setBuffer(bufferBlue);
    laserSoundGreen.setBuffer(bufferGreen);
    explosionSound.setBuffer(bufferExplosion);
    rechargeSound.setBuffer(bufferRecharge);

    // Textures
    tExplosionShip.loadFromFile("images/explosions/type_B.png");
    sExplosionShip.set(tExplosionShip, 0, 0, 192, 192, 64, 0.5);

    tPlayerBlue.loadFromFile("images/blueship.png");
    tPlayerBlue.setSmooth(true);
    sPlayerBlue.set(tPlayerBlue, 40, 0, 40, 40, 1, 0);
    sPlayerBlueGo.set(tPlayerBlue, 40, 40, 40, 40, 1, 0);
    tBulletBlue.loadFromFile("images/bulletBlue.png");
    sBulletBlue.set(tBulletBlue, 0, 0, 32, 64, 16, 0.8);

    tPlayerGreen.loadFromFile("images/greenship.png");
    tPlayerGreen.setSmooth(true);
    sPlayerGreen.set(tPlayerGreen, 40, 0, 40, 40, 1, 0);
    sPlayerGreenGo.set(tPlayerGreen, 40, 40, 40, 40, 1, 0);
    tBulletGreen.loadFromFile("images/bulletGreen.png");
    sBulletGreen.set(tBulletGreen, 0, 0, 32, 64, 16, 0.8);

    // Players
    playerBlue = new Player("Blue", sPlayerBlueGo, sBulletBlue, laserSoundBlue, rechargeSound, entities);
    playerBlue->settings(sPlayerBlue, 20, resolution.h / 2, 0, 20);
    entities.push_back(playerBlue);

    playerGreen = new Player("Green", sPlayerGreenGo, sBulletGreen, laserSoundGreen, rechargeSound, entities);
    playerGreen->settings(sPlayerGreen, resolution.w - 20, resolution.h / 2, -180, 20);
    entities.push_back(playerGreen);

    players[0] = playerBlue;
    players[1] = playerGreen;
    score.set(font);

    return Screen::Ok;
  }

  virtual int run(RenderWindow& window) {
    bool running = true;

    while (running) {

      Event event;
      while (window.pollEvent(event)) {

        if (event.type == Event::Closed) { return Screen::Exit; }

        if (event.type == Event::KeyPressed) {
          if (event.key.code == Keyboard::Escape) { return Screen::Menu; }
          if (event.key.code == Keyboard::F11) { toggleFullScreen(window); }
        }
        else if ((event.type == Event::JoystickButtonPressed) ||
          (event.type == Event::JoystickButtonReleased) ||
          (event.type == Event::JoystickMoved) ||
          (event.type == Event::JoystickConnected)) {

          // Update displayed joystick values
          int id = event.joystickConnect.joystickId;
          players[id]->joystick(id);
          score.updateBlue(playerBlue->score, playerBlue->bullets);
          score.updateGreen(playerGreen->score, playerGreen->bullets);
        }
        else if (event.type == Event::JoystickDisconnected) {
          ;
        }
      } // pollEvent

      for (auto e : entities) {
        if (e->name == "explosion") {
          if (e->anim.isEnd()) { e->life = 0; }
        }
      }
      for (auto a : entities) {
        for (auto b : entities) {
          if (a->name == "Green" && b->name == "bulletBlue" ||
            a->name == "Blue" && b->name == "bulletGreen") {

            if (isCollide(a, b)) {
              explosionSound.play();
              Entity* e = new Entity();
              e->settings(sExplosionShip, a->x, a->y);
              e->name = "explosion";
              entities.push_back(e);

              a->settings(a->animQuiet, rand() % resolution.w, rand() % resolution.h, 0, 20);
              a->dx = 0; a->dy = 0;
              ((Player*)a)->score--;
              score.updateBlue(playerBlue->score, playerBlue->bullets);
              score.updateGreen(playerGreen->score, playerGreen->bullets);
            }
          }
          if (a->name == "Green" && b->name == "Blue") {
            if (isCollide(a, b)) {
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

              a->settings(a->animQuiet, rand() % resolution.w, rand() % resolution.h, 0, 20);
              a->dx = 0; a->dy = 0;
              ((Player*)a)->score--;
              b->settings(b->animQuiet, rand() % resolution.w, rand() % resolution.h, 0, 20);
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
      global.draw(window);

      for (auto e : entities) {
        e->draw(window);
      }

      score.draw(window);
      window.display();

    }
    return 0;
  }
};

int main()
{
  srand(time(0));

  Pair res = { 800, 600 };
  printf("%i %i\n", res.x, res.y);
  res.w = 1024; res.h = 768;
  printf("%i %i\n", res.x, res.y);

  Global global; global.init();

  printf("Chosen resolution: %i %i\n", resolution.w, resolution.h);

  Music music;
  std::string s("sounds/spacemusic1.ogg");
  //if (!music.openFromFile("sounds/spacemusic1.ogg"))
  if (!music.openFromFile(s)) { return 1; }
  music.setLoop(true);
  music.play();


  RenderWindow window(VideoMode(resolution.w, resolution.h), "Asteroids!", Style::Resize);
  window.setFramerateLimit(60);

  Menu menu(global); int result = menu.init(); if (result == -1) { exit(1); }
  Game game(global); result = game.init(); if (result == -1) { exit(1); }

  Screen* screens[2];
  screens[0] = &menu;
  screens[1] = &game;
  int currentScreen = Screen::Menu;

  while (currentScreen >= 0) {
    currentScreen = screens[currentScreen]->run(window);
  }

  music.stop();
  return 0;
}
