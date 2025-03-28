#include <iostream>
#include <math.h>
#include <windows.h>
#include <conio.h>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <string>
using namespace std;

#define PI 3.14159265
#define g 1.625
#define g0 9.81

#define SLS_X 4  
#define SLS_Y 7  
//safe landing speed

#define MOON_GROUND '~'
#define TERRAIN_WIDTH 3500
#define TERRAIN_HEIGHT 3
#define MAX_MAP_HEIGHT 2000

#define TIMESTEP 0.06

int WIDTH = 120;
int HEIGHT = 30;
int LANDER_SCREEN_X = WIDTH / 2;
int LANDER_SCREEN_Y = HEIGHT / 3;

enum ConsoleColors {
    BLACK = 0,
    DARK_BLUE = FOREGROUND_BLUE,
    DARK_GREEN = FOREGROUND_GREEN,
    DARK_CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE,
    DARK_RED = FOREGROUND_RED,
    DARK_MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE,
    DARK_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN,
    GRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    DARK_GRAY = FOREGROUND_INTENSITY,
    BLUE = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
    GREEN = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    CYAN = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
    RED = FOREGROUND_INTENSITY | FOREGROUND_RED,
    MAGENTA = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
    YELLOW = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    WHITE = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE

};

void gotoxy(int x, int y) {
	COORD pos = { x, y };
	HANDLE houtput = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(houtput, pos);
}

bool isValidX(int x) {
	return x >= 0 && x < WIDTH;
}

bool isValidY(int y) {
	return y >= 1 && y < HEIGHT;
}

constexpr double deg2rad(double deg) {
	return deg * (PI / 180.0);
}

constexpr double rad2deg(double rad) {
	return rad * (180.0 / PI);
}

struct Point
{
	double x, y;
};

void showcursor(bool showFlag)
{
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO     cursorInfo;

	GetConsoleCursorInfo(out, &cursorInfo);
	cursorInfo.bVisible = showFlag;
	SetConsoleCursorInfo(out, &cursorInfo);
}

class Blueprint {
public:
	vector<vector<char>> blueprint;
	int width;
	int height;

	static vector<Blueprint> LMblueprints;

	Blueprint() {
		width = 0;
		height = 0;
	}

	Blueprint(int width, int height) {
		this->width = width;
		this->height = height;
		blueprint.resize(height);
		for (int i = 0; i < height; i++) {
			blueprint[i].resize(width);
			for (int j = 0; j < width; j++) {
				blueprint[i][j] = ' ';
			}
		}
	}

	Blueprint(int width, int height, vector<vector<char>> blueprint) {
		this->width = width;
		this->height = height;
		this->blueprint = blueprint;
	}

	void print(int x, int y) {
		for (int i = 0; i < height; i++) {
			gotoxy(x, y + i);
			for (int j = 0; j < width; j++) {
				cout << blueprint[i][j];
			}
			cout << endl;
		}
	}
	void printLM(int x, int y, int angle, bool isThrustOn, bool hasFuel) {
		for (int i = 0; i < height; i++) {
			gotoxy(x, y + i);
			for (int j = 0; j < width; j++) {
				if (isThrustOn && hasFuel) {
					switch (angle) {
					case 0:
						if (i == 2 && j == 2) {
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), RED);
							cout << blueprint[i][j];
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);
						} else {
							cout << blueprint[i][j];
						}
						break;
					case 90:
						if (i == 1 && j == 4) {
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), RED);
							cout << blueprint[i][j];
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);
						}
						else {
							cout << blueprint[i][j];
						}
						break;
					case -90:
						if (i == 1 && j == 0) {
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), RED);
							cout << blueprint[i][j];
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);
						}
						else {
							cout << blueprint[i][j];
						}
						break;
					default:
						cout << blueprint[i][j];
						break;
					}

				}
				else {
					cout << blueprint[i][j];
				}
			}
			cout << endl;
		}
	}
};

vector<Blueprint> Blueprint::LMblueprints = {
	Blueprint(5, 3, {
		{' ', '_', '_', '_', ' '},
		{'[', '_', '_', '_', ']'},
		{'/', ' ', ' ', ' ', '\\'}
	}),
	Blueprint(5, 3, {
		{' ', ' ', ' ', '-', '/'},
		{' ', ' ', '|', '|', ' '},
		{' ', ' ', ' ', '-', '\\'},
	}),
	Blueprint(5, 3, {
		{'\\', '-', ' ', ' ', ' '},
		{' ', '|', '|', ' ', ' '},
		{'/', '-', ' ', ' ', ' '}
	})
};

class DescentEngine {
private:
	double Isp100p;
	double Isp10p;
	double maxThrust;
	double maxFuelFlowRate;
public:
	double percentThrust;
	double currentThrust;
	double currentIsp;
	double currentFuelFlowRate;
	char thrustIndicator;

	DescentEngine() {
		Isp100p = 311;
		Isp10p = 285;
		maxThrust = 47000;
		percentThrust = 30;
		maxFuelFlowRate = 30;
		changeThrust(percentThrust);
	}

	void changeThrust(double percent) {
		if (percent < 0) percent = 0;
		if (percent > 100) percent = 100;

		if (percent == 0) {
			thrustIndicator = ' ';
		}
		percentThrust = percent;
		currentThrust = maxThrust * percentThrust / 100;
		
		changethrustIndicator(percentThrust);

		changeIsp(percentThrust);
		changeFuelFlowRate(percentThrust);
	}

	void changethrustIndicator(double percent) {
		if (percent <= 0) {
			thrustIndicator = ' ';
		}
		else if (percent < 25) {
			thrustIndicator = '.';
		}
		else if (percent < 50) {
			thrustIndicator = ':';
		}
		else if (percent < 75) {
			thrustIndicator = 'w';
		}
		else {
			thrustIndicator = 'W';
		}
	}

	void changeFuelFlowRate(double percent) {
		if (percent < 0) percent = 0;
		if (percent > 100) percent = 100;
		currentFuelFlowRate = maxFuelFlowRate * percent / 100;
	}

	void changeIsp(double percent) {
		if (percent < 0) percent = 0;
		if (percent > 100) percent = 100;

		currentIsp = Isp100p - (Isp100p - Isp10p) * (100 - percent) / 100;
	}
};

class RCS {
public:
	double thrust;
	int turnedOn = 0;
	RCS() {
		thrust = 20000; //unrealistic but for the sake of the game
	}

	RCS(double thrust) {
		this->thrust = thrust;
	}
};

class Lander {
private:
	double startFuelMass;
	Point velocity;
	double dryMass;
	double fuelMass;
	double accX;
	double accY;

	DescentEngine engine;
	RCS rcsLeft = RCS();
	RCS rcsRight = RCS();

	double angle; // degrees
public:
	bool landed = false;
	bool crashed = false;
	Blueprint currentBlueprint;
	Point position;
	double dV;
	double deltaTime = TIMESTEP;

	Lander() {
		position.x = TERRAIN_WIDTH / 2;
		position.y = MAX_MAP_HEIGHT - 460;
		velocity.x = 15;
		velocity.y = 0;

		dryMass = 6800;
		startFuelMass = 8200;
		fuelMass = 620;
		engine = DescentEngine();
		currentBlueprint = Blueprint::LMblueprints[0];
		calculateDV();
		calculateAcceleration();

		changeAngle(0);
		changeThrust(0);
	}

	Lander(int posX, int posY, int velX, int velY, double fm) {
		position.x = posX;
		position.y = posY;
		velocity.x = velX;
		velocity.y = velY;

		dryMass = 6800;
		startFuelMass = 8200;
		fuelMass = fm;
		engine = DescentEngine();
		currentBlueprint = Blueprint::LMblueprints[0];
		calculateDV();
		calculateAcceleration();

		changeAngle(0);
		changeThrust(0);
	}

	void changeAngle(int degrees) {
		clear();
		switch (degrees) {
		case 90:
			angle = 90;
			currentBlueprint = Blueprint::LMblueprints[1];
			break;
		case -90:
			angle = -90;
			currentBlueprint = Blueprint::LMblueprints[2];
			break;
		default:
			angle = 0;
			currentBlueprint = Blueprint::LMblueprints[0];
			break;
		}
		changeThrust(engine.percentThrust);
	}

	void changeThrust(double percent) {
		if (fuelMass == 0) {
			return;
		}
		engine.changeThrust(percent);
		if (angle == 0) {
			if (fuelMass != 0)
			currentBlueprint.blueprint[2][2] = engine.thrustIndicator;
		}
		else if (angle == 90) {
			if (percent != 0) currentBlueprint.blueprint[1][4] = '>';
			else currentBlueprint.blueprint[1][4] = ' ';
		}
		else if (angle == -90) { 
			if (percent != 0) currentBlueprint.blueprint[1][0] = '<';
			else currentBlueprint.blueprint[1][0] = ' ';
		}
		else {
			currentBlueprint.blueprint[2][2] = engine.thrustIndicator;
		}
	}

	void update(bool isCollided) {
		if (isCollided) {
			if (abs(velocity.x) <= SLS_X && velocity.y <= SLS_Y + 1) {
				landed = true;
			}
			else {
				crashed = true;  
			}
			accX = 0;
			accY = 0;
			velocity.x = 0;
			velocity.y = 0;
		}	
		else {
			
			velocity.x += accX * deltaTime;
			velocity.y += accY * deltaTime;
			position.x += velocity.x * deltaTime;
			position.y += velocity.y * deltaTime;

			if (fuelMass <= 0) {
				engine.changeThrust(0);
			}
			fuelMass -= engine.currentFuelFlowRate * deltaTime;
			calculateDV();
			calculateAcceleration();
		}
	}

	void calculateDV() {
		dV = engine.currentIsp * g0 * log((dryMass + fuelMass) / dryMass); // Isp * g0 * ln(m0/m1)
	}

	void calculateAcceleration() {
		double totalMass = dryMass + fuelMass;

		accX = (engine.currentThrust * sin(deg2rad(angle))) / totalMass;
		if (angle != 0) { accX = -accX; }
		accX += -((rcsRight.turnedOn - rcsLeft.turnedOn) * rcsLeft.thrust / totalMass);

		accY = -((engine.currentThrust * cos(deg2rad(angle))) / totalMass - g);
	}

	void turnRCSLeft() {
		if (angle == 0) {
			currentBlueprint.blueprint[0][0] = '<';
		}
		rcsLeft.turnedOn = 1;
	}

	void turnRCSRight() {
		if (angle == 0) {
			currentBlueprint.blueprint[0][4] = '>';
		}
		rcsRight.turnedOn = 1;
	}

	void turnRCSOff() {
		if (angle == 0) {
			currentBlueprint.blueprint[0][0] = ' ';
			currentBlueprint.blueprint[0][4] = ' ';
		}
		rcsLeft.turnedOn = 0;
		rcsRight.turnedOn = 0;
	}

	void print() {
		currentBlueprint.printLM(LANDER_SCREEN_X, LANDER_SCREEN_Y, this->angle, engine.percentThrust == 0 ? false : true, fuelMass == 0 ? false : true);
	}

	double getIsp() const {
		return engine.currentIsp;
	}

	double getAngle() const {
		return angle;
	}

	DescentEngine getEngine() const {
		return engine;
	}

	Point getVelocity() const {
		return velocity;
	}	

	double getAltitude() const {
		return MAX_MAP_HEIGHT - int(position.y) - currentBlueprint.height - TERRAIN_HEIGHT;
	}

	void showData(bool isCollided) const {
		if (WIDTH == 209 && HEIGHT == 51) gotoxy(147, 14);
		else gotoxy(LANDER_SCREEN_X - 100, 0);
		cout << "Vel X: " << int(velocity.x) << "; Vel Y: " << -int(velocity.y) << "; Thrust: " << engine.percentThrust << "%" << "; Fuel: " << int((fuelMass / startFuelMass) * 100) << "%" << "; Altitude: " << int(getAltitude()) << " m; " << endl;
	}

	void clearData() const {
		for (int i = 0; i < WIDTH; i++) {
			gotoxy(i, 0);
			cout << " ";
		}
	}

	void clear() {
		Blueprint empty(currentBlueprint.width,
			currentBlueprint.height);
		empty.print(LANDER_SCREEN_X, LANDER_SCREEN_Y);
	}
};

class MoonTerrain {
private:
	vector<vector<char>> tiles;
	Point position;
	Point consolePosition;

public:
	Point relativePosition;
	MoonTerrain() {
		position.x = 0;
		position.y = MAX_MAP_HEIGHT - TERRAIN_HEIGHT + 1;

		tiles.resize(TERRAIN_HEIGHT, vector<char>(TERRAIN_WIDTH, ' '));

		const int BASE_HEIGHT = TERRAIN_HEIGHT - 3;
		const double WAVE_FREQUENCY = 0.08;
		const double CRATER_CHANCE = 0.05;  

		for (int x = 0; x < TERRAIN_WIDTH; x++) {
			double sineValue = sin(x * WAVE_FREQUENCY);
			int height = BASE_HEIGHT + static_cast<int>(sineValue * 3);

			int randomVariation = (rand() % 3 - 1); 
			height += randomVariation;

			if (height < 0) height = 0;
			if (height >= TERRAIN_HEIGHT) height = TERRAIN_HEIGHT - 1;
			

			for (int y = height; y < TERRAIN_HEIGHT; y++) {
				tiles[y][x] = MOON_GROUND;
			}
		}
	}

	bool checkCollision(Lander& lander) {
		
		int landerLeft = lander.position.x;
		int landerRight = lander.position.x + lander.currentBlueprint.width; 
		int landerBottom = int(lander.position.y) + lander.currentBlueprint.height;
		int landerTop = lander.position.y;

		int landerVerticalSpeed = lander.getVelocity().y;

		for (int i = 0; i < TERRAIN_HEIGHT; i++) {
			for (int j = landerLeft - 1; j <= landerRight + 1; j++) {
				if (tiles[i][j] == MOON_GROUND) { 
					int tileX = j;
					int tileY = position.y + i;
					int predictedBottom = landerBottom + landerVerticalSpeed*TIMESTEP*8.8;

					if (tileX >= landerLeft && tileX <= landerRight && (tileY <= landerBottom - 2 || tileY <= predictedBottom - 1)) { //asd
						return true;
					}

					if (tileX == landerLeft - 1 && tileY >= landerTop && tileY <= landerBottom - 2) return true;
					else if (tileX == landerRight + 1 && tileY >= landerTop && tileY <= landerBottom - 2) return true;//
				}
			}
		}
		return false;
	}

	void print() {
		for (int i = 0; i < TERRAIN_HEIGHT; i++) {
			if (!isValidY(i + consolePosition.y)) continue;
			gotoxy(0, i + consolePosition.y);
			for (int j = int(-relativePosition.x); j < int(-relativePosition.x) + WIDTH; j++) {
				cout << tiles[i][j];
			}
		}
	}

	void clear() {
		for (int i = 0; i < TERRAIN_HEIGHT; i++) { 
			if (!isValidY(i + consolePosition.y)) continue;
			gotoxy(0, i + consolePosition.y);
			for (int j = int(-relativePosition.x); j < int(-relativePosition.x) + WIDTH; j++) {
				cout << ' ';
			}
		}
	}

	void updateConsolePosition(Lander& lander) {
		if (checkCollision(lander)) {
			return;
		}
		clear();
		relativePosition.x = -lander.position.x; 
		relativePosition.y = this->position.y - lander.position.y;

		consolePosition.x = LANDER_SCREEN_X + relativePosition.x;
		consolePosition.y = LANDER_SCREEN_Y + relativePosition.y;
	}
};

class TrajectoryDisplay {
private:
	const int width = 50;
	const int height = 13;
	Point position;
	vector<vector<char>> buffer;
public:
	TrajectoryDisplay(int x, int y) : position{ (double)x, (double)y } {
		buffer.resize(height, vector<char>(width, ' '));
		drawBorder();
	}

	void drawBorder() {
		for (int x = 0; x < width; x++) {
			buffer[0][x] = '-';
			buffer[height - 1][x] = '-';
		}
		for (int y = 1; y < height - 1; y++) {
			buffer[y][0] = '|';
			buffer[y][width - 1] = '|';
		}
		buffer[0][0] = '+';
		buffer[0][width - 1] = '+';
		buffer[height - 1][0] = '+';
		buffer[height - 1][width - 1] = '+';
	}

	void clearGraph() {
		for (int y = 1; y < height - 1; y++) {
			for (int x = 1; x < width - 1; x++) {
				buffer[y][x] = ' ';
			}
		}
	}

	void update(const Lander& lander) {
		clearGraph();

		double scaleX = lander.position.x / TERRAIN_WIDTH;
		double scaleY = lander.position.y / MAX_MAP_HEIGHT; 

		int displayX = 1 + int(scaleX * (width - 2));
		int displayY = 1 + int(scaleY * (height - 2));

		const double timeStep = 0.2;

		Point landerVel = lander.getVelocity();
		double velX = landerVel.x;
		double velY = landerVel.y; 

		for (double t = 0; t < 100.0; t += timeStep) {
			double projWorldX = lander.position.x + velX * t;
			double projWorldY = lander.position.y + velY * t + 0.5 * g * t * t;

			double projDisplayX = projWorldX / TERRAIN_WIDTH;
			double projDisplayY = projWorldY / MAX_MAP_HEIGHT;

			int x = 1 + int(projDisplayX * (width - 2));
			int y = 1 + int(projDisplayY * (height - 2));

			if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
				buffer[y][x] = '.';
			}

			if (projWorldY > MAX_MAP_HEIGHT) break;
		}

		if (displayX > 0 && displayX < width - 1 &&
			displayY > 0 && displayY < height - 1) {
			buffer[displayY][displayX] = 'x';
		}
	}

	void print() {
		for (int y = 0; y < height; y++) {
			gotoxy(position.x, position.y + y);
			for (int x = 0; x < width; x++) {
				cout << buffer[y][x];
			}
		}
	}
};

void setWH() {
	bool start = false;
	gotoxy(LANDER_SCREEN_X - 10, LANDER_SCREEN_Y);
	cout << "Set console size before playing." << endl;
	gotoxy(LANDER_SCREEN_X - 10, LANDER_SCREEN_Y + 1);
	cout << "Press K to start." << endl;
	while (true) {
		if (start) {
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			int columns, rows;

			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
			columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
			rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
			WIDTH = columns;
			HEIGHT = rows;
			LANDER_SCREEN_X = WIDTH / 2;
			LANDER_SCREEN_Y = HEIGHT / 3;

			system("CLS");
			return;
		}
			//75 107
		if (_kbhit()) {
			int key = _getch();
			switch (key) {
			case 75: case 107: 
				start = true;// K
				break;        
			default:
				break;
			}
		}
		Sleep(50);
	}
}

void eraseFile(const string& filename) {
	ofstream file(filename, ios::trunc);
	file.close();
}

class GameLevel {
public:
	static vector<GameLevel> levels;
	int posX;
	int posY;
	int velX;
	int velY;
	double fuelMass;
public:
	string difficulty;
	enum GameState { RUNNING, CRASHED, LANDED, OUT_OF_BOUNDS };

	GameLevel(string dif, int x, int y, int vx, int vy, double fm) : difficulty(dif), posX(x), posY(y), velX(vx), velY(vy), fuelMass(fm) {}
	GameLevel() : GameLevel("EASY", TERRAIN_WIDTH / 2, MAX_MAP_HEIGHT - 20, 0, 0, 1000) {}

	static void printInstruction() {
		gotoxy(LANDER_SCREEN_X - 20, LANDER_SCREEN_Y - 10);
		cout << "Instruction. Press ESC to return to the main menu.";
		Blueprint angle0 = Blueprint::LMblueprints[0];
		Blueprint angle_90 = Blueprint::LMblueprints[1];
		Blueprint angle90 = Blueprint::LMblueprints[2];

		angle0.blueprint[0][0] = '<';
		angle0.blueprint[0][4] = '>';
		angle0.blueprint[2][2] = 'W';
		angle0.printLM(LANDER_SCREEN_X, LANDER_SCREEN_Y, 0, true, true);
		gotoxy(LANDER_SCREEN_X - 2, LANDER_SCREEN_Y - 1); cout << "RCS";
		gotoxy(LANDER_SCREEN_X + 4, LANDER_SCREEN_Y - 1); cout << "RCS";
		gotoxy(LANDER_SCREEN_X + 2, LANDER_SCREEN_Y + 3); cout << "^";

		gotoxy(LANDER_SCREEN_X - 3, LANDER_SCREEN_Y + 4); cout << "ENGINE GAS";

		angle_90.blueprint[1][4] = '>';
		angle_90.printLM(LANDER_SCREEN_X - 15, LANDER_SCREEN_Y, 90, true, true);

		angle90.blueprint[1][0] = '<';
		angle90.printLM(LANDER_SCREEN_X + 15, LANDER_SCREEN_Y, -90, true, true);
		
		gotoxy(3, 2);
		cout << "-------------------------------";
		gotoxy(3, 3);
		cout << "W - increase thrust. S - decrease thrust.";
		gotoxy(3, 4);
		cout << "Z - 100% thrust. X - 0% thrust.";
		gotoxy(3, 5);
		cout << "-------------------------------";
		gotoxy(3, 6);
		cout << "A - left RCS ON. D - right RSC ON";
		gotoxy(3, 7);
		cout << "-------------------------------";
		gotoxy(3, 8);
		cout << "Q - rotate nozzle left. E - rotate nozzle right";

		while (true) {
			if (_kbhit()) {
				showcursor(true);
				int key = _getch();
				if (key == 27) return;
			}
		}
	}

	static void printLevelsItems() {
		gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 5);
		cout << "SELECT LEVEL";
		for (int i = 0; i < GameLevel::levels.size(); i++) {
			gotoxy(LANDER_SCREEN_X - 4, LANDER_SCREEN_Y + i + 2 - 5);
			cout << GameLevel::levels[i].difficulty << " - " << i + 1;
		}
		gotoxy(LANDER_SCREEN_X - 4, LANDER_SCREEN_Y + GameLevel::levels.size() + 3 - 5);
		cout << "INSTRUCTION - I";
		gotoxy(LANDER_SCREEN_X - 4, LANDER_SCREEN_Y + GameLevel::levels.size() + 3 - 4);
		cout << "CREATE LEVEL - C";
		gotoxy(LANDER_SCREEN_X - 4, LANDER_SCREEN_Y + GameLevel::levels.size() + 3 - 3);
		cout << "EXIT - 0" << endl;
	}

	static GameLevel showMainMenu() {
		system("CLS");
		printLevelsItems();
		while (true) {
			if (_kbhit()) {
				int key = _getch();
				switch (key) {
				case 67: case 99:
					system("CLS");
					createLevel();
					system("CLS");
					printLevelsItems();
					break;
				case 73: case 105:
					system("CLS");
					printInstruction();
					system("CLS");
					showcursor(false);
					printLevelsItems();
				}
				for (int i = 0; i < GameLevel::levels.size(); i++) {
					if (key - 48 == i + 1) {
						system("CLS");
						showLevelTrajectory(GameLevel::levels[i], LANDER_SCREEN_X - 25, LANDER_SCREEN_Y);
						showLevelInfo(GameLevel::levels[i], LANDER_SCREEN_X - 25, LANDER_SCREEN_Y + 13);
						gotoxy(LANDER_SCREEN_X - 25, LANDER_SCREEN_Y + 15);
						cout << GameLevel::levels[i].difficulty << " was chosen. Press K to continue or ESC to forfeit." << endl;
						bool forfeited = false;
						while (true) {
							if (forfeited) break;
							if (_kbhit()) {
								int key = _getch();
								switch (key) {
								case 75: case 107:
									system("CLS");
									return GameLevel::levels[i];
								case 27:
									forfeited = true;
								}
							}
						}
						system("CLS");
						printLevelsItems();
					}
				}
				if (key == 48) exit(0);
			}
			Sleep(50);
		}
	}

	static void printCreateLevel(GameLevel& level) {
		gotoxy(LANDER_SCREEN_X - 40, LANDER_SCREEN_Y - 10);
		cout << "Press K to save level or ESC to exit (or E to erase file). Values are currently default";

		gotoxy(LANDER_SCREEN_X - 25, LANDER_SCREEN_Y - 9);
		cout << "Change X - 1:";
		gotoxy(LANDER_SCREEN_X - 25, LANDER_SCREEN_Y - 8);
		cout << "Change altitude - 2:";
		gotoxy(LANDER_SCREEN_X - 25, LANDER_SCREEN_Y - 7);
		cout << "Change vel X - 3:";
		gotoxy(LANDER_SCREEN_X - 25, LANDER_SCREEN_Y - 6);
		cout << "Change vel Y - 4:";
		gotoxy(LANDER_SCREEN_X - 25, LANDER_SCREEN_Y - 5);
		cout << "Change fuel(%) - 5:";
		gotoxy(LANDER_SCREEN_X - 25, LANDER_SCREEN_Y - 4);
		cout << "Input name - 6:";
		showLevelTrajectory(level, LANDER_SCREEN_X - 25, LANDER_SCREEN_Y - 2);
	}

	static void createLevel() {
		GameLevel createdLevel("DEFAULT_NAME", TERRAIN_WIDTH / 2, MAX_MAP_HEIGHT - 10, -0, 0, 8200);
		
		printCreateLevel(createdLevel);
		while (true) {
			if (_kbhit()) {
				showcursor(true);
				int key = _getch();
				switch (key) {
				case 27: system("CLS"); showcursor(false); return;
				case 101: case 69: // 'e'
					eraseFile("AdditionalLevels.txt");
					break;
				case 75: case 107:
					system("CLS");
					showcursor(false);
					if (GameLevel::levels.size() != 9) {
						writeLevelToFile(createdLevel, "AdditionalLevels.txt");
						GameLevel::levels.push_back(createdLevel);
						createdLevel = GameLevel("DEFAULT_NAME", TERRAIN_WIDTH / 2, MAX_MAP_HEIGHT - 10, -0, 0, 8200);
					}
					else {
						gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y);
						cout << "All slots are full.";
						Sleep(1500);
						for (int i = 0; i < 10; i++) {
							gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y);
							cout << ' ';
						}
					}
					break;
				case 49:
					for (int i = 0; i < 10; i++) {
						gotoxy(LANDER_SCREEN_X - 5 + i, LANDER_SCREEN_Y - 9);
						cout << ' ';
					}
					gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 9);
					int x;
					cin >> x;
					createdLevel.changePosX(x);
					break;
				case 50:
					for (int i = 0; i < 10; i++) {
						gotoxy(LANDER_SCREEN_X - 5 + i, LANDER_SCREEN_Y - 8);
						cout << ' ';
					}
					gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 8);
					int alt;
					cin >> alt;
					createdLevel.changeAltitude(alt + 3);
					break;
				case 51:
					for (int i = 0; i < 10; i++) {
						gotoxy(LANDER_SCREEN_X - 5 + i, LANDER_SCREEN_Y - 7);
						cout << ' ';
					}
					gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 7);
					int velx;
					cin >> velx;
					createdLevel.changeVelX(velx);
					break;
				case 52:
					for (int i = 0; i < 10; i++) {
						gotoxy(LANDER_SCREEN_X - 5 + i, LANDER_SCREEN_Y - 6);
						cout << ' ';
					}
					gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 6);
					int vely;
					cin >> vely;
					createdLevel.changeVelY(vely); 
					break;
				case 53:
					for (int i = 0; i < 10; i++) {
						gotoxy(LANDER_SCREEN_X - 5 + i, LANDER_SCREEN_Y - 5);
						cout << ' ';
					}
					gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 5);
					int f;
					cin >> f;
					createdLevel.changeFuel(f);
					break;
				case 54:
					for (int i = 0; i < 10; i++) {
						gotoxy(LANDER_SCREEN_X - 5 + i, LANDER_SCREEN_Y - 4);
						cout << ' ';
					}
					gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 4);
					string n;
					cin >> n;
					createdLevel.difficulty = n;
					break;
				}
				showcursor(false);
				printCreateLevel(createdLevel);
			}
			Sleep(50);
		}
	}

	friend static void showLevelInfo(GameLevel& level, int x, int y) {
		gotoxy(x, y);
		cout << "Fuel: " << int((level.fuelMass / 8200) * 100) << " %" << "; Altitude: " << MAX_MAP_HEIGHT - level.posY - 3 << " m; ";
		cout << "Vx: " << level.velX << " m/s; " << "Vy: " << -level.velY << " m/s." << endl;
	}

	friend static void showLevelTrajectory(GameLevel& level, int x, int y) {
		Lander lander(level.posX, level.posY, level.velX, level.velY, level.fuelMass);
		TrajectoryDisplay trajectory(x, y); 
		trajectory.update(lander);
		trajectory.print();
	}

	bool handleInput(Lander& lander) {
		if (_kbhit()) {
			int key = _getch();
			switch (key) {
			case 27: return true; // pause
			case 119: case 87: // 'w'
				lander.changeThrust(lander.getEngine().percentThrust + 5);
				break;
			case 115: case 83: // 's'
				lander.changeThrust(lander.getEngine().percentThrust - 5);
				break;
			case 97: case 65:  // 'a'
				lander.turnRCSRight();
				break;
			case 100: case 68: // 'd'
				lander.turnRCSLeft();
				break;
			case 122: case 90: // 'z'
				lander.changeThrust(100);
				break;
			case 120: case 88: // 'x'
				lander.changeThrust(0);
				break;
			case 113: case 81: // 'q'
				lander.changeAngle(lander.getAngle() - 90);
				break;
			case 101: case 69: // 'e'
				lander.changeAngle(lander.getAngle() + 90);
				break;
			default:
				lander.turnRCSOff();
				break;
			}
		}
		else {
			lander.turnRCSOff();
		}
		return false;
	}

	GameState run() {
		MoonTerrain terrain;
		Lander lander(posX, posY, velX, velY, fuelMass);
		TrajectoryDisplay trajectory(WIDTH - 50, 0);
		terrain.updateConsolePosition(lander);

		while (true) {
			if (lander.position.x <= 1 || lander.position.x >= TERRAIN_WIDTH - 1 ||
				lander.position.y <= 1) {
				gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 10);
				cout << "You went out of the map bounds" << endl;
				Sleep(2000);
				return OUT_OF_BOUNDS;
			}
			if (lander.crashed) { 
				gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 5);
				cout << "CRASH!" << endl;
				Sleep(2000);
				return CRASHED;
			}
			if (lander.landed) {
				gotoxy(LANDER_SCREEN_X - 5, LANDER_SCREEN_Y - 5);
				cout << "Landed successfully!" << endl;
				Sleep(2000);
				return LANDED;
			}

			bool isCollided = terrain.checkCollision(lander);
			lander.clearData();
			lander.showData(isCollided);

			terrain.print();
			lander.print();

			trajectory.update(lander);
			trajectory.print();

			terrain.updateConsolePosition(lander);
			lander.update(isCollided);

			if (handleInput(lander)) showPauseMenu(lander);
			Sleep(1);
		}
	}

	static void showEndMenu(GameState state) {
		system("CLS");
		gotoxy(LANDER_SCREEN_X - 10, LANDER_SCREEN_Y);

		switch (state) {
		case CRASHED: cout << "CRASH LANDING!"; break;
		case LANDED: cout << "SUCCESSFUL LANDING!"; break;
		case OUT_OF_BOUNDS: cout << "OUT OF BOUNDS!"; break;
		}

		gotoxy(LANDER_SCREEN_X - 10, LANDER_SCREEN_Y + 2);
		cout << "1. Main Menu";
		gotoxy(LANDER_SCREEN_X - 10, LANDER_SCREEN_Y + 3);
		cout << "2. Exit";

		while (true) {
			if (_kbhit()) {
				int key = _getch();
				switch (key) {
				case 49: return; // restart
				case 50: exit(0);
				}
			}
			Sleep(50);
		}
	}

	void showPauseMenu(Lander& lander) {
		system("CLS");
		gotoxy(LANDER_SCREEN_X - 8, LANDER_SCREEN_Y);
		cout << "GAME PAUSED";
		gotoxy(LANDER_SCREEN_X - 10, LANDER_SCREEN_Y + 2);
		cout << "1. Continue";
		gotoxy(LANDER_SCREEN_X - 10, LANDER_SCREEN_Y + 3);
		cout << "2. End Level";
		gotoxy(LANDER_SCREEN_X - 10, LANDER_SCREEN_Y + 4);
		cout << "3. Exit";

		while (true) {
			if (_kbhit()) {
				int key = _getch();
				switch (key) {
				case 49: system("CLS"); return;
				case 50: 
					showMainMenu();
					lander = Lander(posX, posY, velX, velY, fuelMass);
					return;
				case 51: exit(0);
				}
			}
			Sleep(50);
		}
	}

	static void writeLevelToFile(const GameLevel& level, const string& filename) {
		ofstream outFile(filename, ios::app);
		if (outFile.is_open()) {
			outFile << level.posX << " "
				<< level.posY << " "
				<< level.velX << " "
				<< level.velY << " "
				<< level.fuelMass << " "
				<< level.difficulty << "\n";
			outFile.close();
		}
	}

	static void addToLevels(vector<GameLevel> newLevels) {
		for (auto l : newLevels) {
			GameLevel::levels.push_back(l);
		}
	}

	void changePosX(int newPosX) {
		if (newPosX <= 1 || newPosX >= TERRAIN_WIDTH - 1) return;
		this->posX = newPosX;
	}
	void changeAltitude(int newAltitude) {
		if (newAltitude <= 8 || newAltitude >= MAX_MAP_HEIGHT - 8);
		this->posY = MAX_MAP_HEIGHT - newAltitude;
	}
	void changeVelX(int newVelX) {
		this->velX = newVelX;
	}
	void changeVelY(int newVelY) {
		this->velY = -newVelY;
	}
	void changeFuel(int percent) {
		if (percent <= 0 || percent > 100) return;
		this->fuelMass = int((8200 * percent)/100);
	}
};

vector<GameLevel> readLevelsFromFile(const string& filename) {
	vector<GameLevel> levels;
	ifstream inFile(filename);

	if (inFile.is_open()) {
		GameLevel level;
		while (inFile >> level.posX
			>> level.posY
			>> level.velX
			>> level.velY
			>> level.fuelMass
			>> level.difficulty) {
			levels.push_back(level);
		}
		inFile.close();
	}

	return levels;
}

vector<GameLevel> GameLevel::levels = {
	GameLevel("EASY", TERRAIN_WIDTH / 2, MAX_MAP_HEIGHT - 200, 1, 0, 4000),
	GameLevel("MEDIUM", TERRAIN_WIDTH / 4, MAX_MAP_HEIGHT - 500, 15, -20, 1000),
	GameLevel("HARD", TERRAIN_WIDTH / 1.5, MAX_MAP_HEIGHT - 760, -10, -20, 300),  
	GameLevel("NEIL_ARMSTRONG", TERRAIN_WIDTH / 1.4, MAX_MAP_HEIGHT - 20, -70, -10, 300)
};

int main() { 
	srand(time(NULL));
	showcursor(false); 
	vector<GameLevel> levels = readLevelsFromFile("AdditionalLevels.txt");
	GameLevel::addToLevels(levels);

	setWH();

	while (true) {
		GameLevel level = GameLevel::showMainMenu();
		GameLevel::GameState result = level.run();
		GameLevel::showEndMenu(result);
	}

	return 0;
}
