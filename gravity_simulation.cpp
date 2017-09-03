#include <GLUT/glut.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <vector>

// 3次元ベクトル
class Vector3d {
public:
	double x, y, z;
	Vector3d() { x = y = z = 0; }
	Vector3d(double _x, double _y, double _z) { x = _x; y = _y; z = _z; }
	void set(double _x, double _y, double _z) { x = _x; y = _y; z = _z; }

	// 長さを1に正規化する
	void normalize() {
		double len = length();
		x /= len; y /= len; z /= len;
	}

	// 長さを返す
	double length() { return sqrt(x * x + y * y + z * z); }

	// s倍する
	void scale(const double s) { x *= s; y *= s; z *= s; }

	// 加算の定義
	Vector3d operator+(Vector3d v) { return Vector3d(x + v.x, y + v.y, z + v.z); }

	// 減算の定義
	Vector3d operator-(Vector3d v) { return Vector3d(x - v.x, y - v.y, z - v.z); }

	// 内積の定義
	double operator*(Vector3d v) { return x * v.x + y* v.y + z * v.z; }

	// 外積の定義
	Vector3d operator%(Vector3d v) { return Vector3d(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }

	// 代入演算の定義
	Vector3d& operator=(const Vector3d& v){ x = v.x; y = v.y; z = v.z; return (*this); }

	// 加算代入の定義
	Vector3d& operator+=(const Vector3d& v) { x += v.x; y += v.y; z += v.z; return (*this); }

	// 減算代入の定義
	Vector3d& operator-=(const Vector3d& v) { x -= v.x; y -= v.y; z -= v.z; return (*this); }

	// 値を出力する
	void print() { printf("Vector3d(%f %f %f)\n", x, y, z); }
};
// マイナスの符号の付いたベクトルを扱えるようにするための定義 例：b=(-a); のように記述できる
Vector3d operator-( const Vector3d& v ) { return( Vector3d( -v.x, -v.y, -v.z ) ); }

// ベクトルと実数の積を扱えるようにするための定義 例： c=5*a+2*b; c=b*3; のように記述できる
Vector3d operator*( const double& k, const Vector3d& v ) { return( Vector3d( k*v.x, k*v.y, k*v.z ) );}
Vector3d operator*( const Vector3d& v, const double& k ) { return( Vector3d( v.x*k, v.y*k, v.z*k ) );}

// ベクトルを実数で割る操作を扱えるようにするための定義 例： c=a/2.3; のように記述できる
Vector3d operator/( const Vector3d& v, const double& k ) { return( Vector3d( v.x/k, v.y/k, v.z/k ) );}

// 質点
class Point {
public:
	Vector3d f; // 質点に働く力のベクトル
	Vector3d v; // 速度ベクトル
	Vector3d p; // 位置
	bool bFixed; // true: 固定されている false:固定されていない
};

// バネ
class Spring {
public:
	Point *p0; // 質点
	Point *p1; // 質点
	double restLength; // 自然長

	Spring(Point *_p0, Point *_p1) {
		p0 = _p0;
		p1 = _p1;
		restLength = (p0->p - p1->p).length();
	}
};

#define POINT_NUM 20

// 布の定義
class Cloth {
public:
	Point points[POINT_NUM][POINT_NUM];
	std::vector<Spring*> springs;

	Cloth() {
		// 質点の定義
		for(int y = 0; y < POINT_NUM; y++) {
			for(int x = 0; x < POINT_NUM; x++) {
				points[x][y].bFixed = false;
				points[x][y].p.set(x - POINT_NUM /2, POINT_NUM /2, -y);
			}
		}

		// バネの設定
		for(int y = 0; y < POINT_NUM; y++) {
			for(int x = 0; x < POINT_NUM; x++) {
				// 横方向のバネ
				if(x < POINT_NUM - 1) {
					springs.push_back(new Spring(&points[x][y], &points[x+1][y]));
				}

				// 縦方向のバネ
				if(y < POINT_NUM -1) {
					springs.push_back(new Spring(&points[x][y], &points[x][y+1]));
				}

				// 右下方向のバネ
				if(x < POINT_NUM - 1 && y < POINT_NUM - 1) {
					springs.push_back(new Spring(&points[x][y], &points[x+1][y+1]));
				}

				// 左下方向のバネ
				if(x > 0 && y < POINT_NUM - 1) {
					springs.push_back(new Spring(&points[x][y], &points[x-1][y+1]));
				}
			}
		}

		// 固定点の指定                           //オプションではいらない
		//points[0][0].bFixed = true;
		//points[POINT_NUM-1][0].bFixed = true;
	}
};


Cloth *cloth;
double rotateAngleH_deg; // 画面水平方向の回転角度
double rotateAngleV_deg; // 縦方向の回転角度
int preMousePositionX;   // マウスカーソルの位置を記憶しておく変数
int preMousePositionY;   // マウスカーソルの位置を記憶しておく変数
bool bRunning; // アニメーションの実行/停止を切り替えるフラグ

double Ks = 8;   // バネ定数
double Mass = 30; // 質点の質量
double dT = 1; // 時間刻み幅
double Dk = 0.1; // 速度に比例して、逆向きにはたらく抵抗係数
Vector3d gravity(0, -0.002, 0); // 重力(y軸方向の負の向きに働く)

void drawCloth(void) {

	// バネを描画
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_LINES);
	for(int i = 0; i < cloth->springs.size(); i++) {
		glVertex3d(cloth->springs[i]->p0->p.x, cloth->springs[i]->p0->p.y, cloth->springs[i]->p0->p.z);
		glVertex3d(cloth->springs[i]->p1->p.x, cloth->springs[i]->p1->p.y, cloth->springs[i]->p1->p.z);
	}
	glEnd();

	// 質点を描画
	glColor3f(1.0f, 0.0f, 0.0f);
	glPointSize(4.0f);
	glBegin(GL_POINTS);
	for(int y = 0; y < POINT_NUM; y++) {
		for(int x = 0; x < POINT_NUM; x++) {
			glVertex3d(cloth->points[x][y].p.x, cloth->points[x][y].p.y, cloth->points[x][y].p.z);
		}
	}
	glEnd();
}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING);
	glLoadIdentity();
	glTranslated(0, 0.0, -50);

	glRotated(rotateAngleV_deg, 1.0, 0.0, 0.0);
	glRotated(rotateAngleH_deg, 0.0, 1.0, 0.0);

	drawCloth();

	glFlush();
}

void resize(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0, (double)w / (double)h, 1.0, 100.0);
	glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {

	if (key == '\033' || key == 'q') { exit(0);} // ESC または q で終了
	if (key == 'a') { bRunning = !bRunning; }    // a でアニメーションのオンオフ
}

void mouse(int button, int state, int x, int y) {
	switch (button) {
  case GLUT_LEFT_BUTTON:
	  preMousePositionX = x;
	  preMousePositionY = y;
	  break;
  case GLUT_MIDDLE_BUTTON:
	  break;
  case GLUT_RIGHT_BUTTON:
	  preMousePositionX = x;
	  preMousePositionY = y;
	  break;
  default:
	  break;
	}
}

// 布の形状の更新
void updateCloth(void) {

  // 全ての質点について順番に処理する
  for(int y = 0; y < POINT_NUM; y++) {
    for(int x = 0; x < POINT_NUM; x++) {
      cloth->points[x][y].f.set(0, 0, 0);
    }
  }

  // バネを1つ1つを見て、その両端の質点に力を加算していくアプローチをとる
  for(int i = 0; i < cloth->springs.size(); i++) {// 全てのバネについて順番に処理する
    Spring *spring = cloth->springs[i];

    // バネの自然長は spring->restLength に格納されている
    // 現在の長さは、両端の質点間の距離を計算して求める
    double length = (spring->p0->p - spring->p1->p).length();

    double length_sub = spring->restLength - length;

    // （自然長 - 現在の長さ）にバネ定数 Ks を掛けた値が求める大きさ
    double size_f = Ks * length_sub;

    // バネには、両端に質点がついているので、一方から他方に向かう方向が力の向き
    Vector3d dir_f = spring->p0->p - spring->p1->p; //p1->p0
    dir_f.normalize();

    // spring->p0->f バネの一方の質点に加わる力を表すので、これに力ベクトルを加える（向きに注意）
    // spring->p1->f バネのもう一方の質点に加わる力を表すので、これに力ベクトルを加える（向きに注意）
    spring->p0->f += (size_f * dir_f);  //p0とp1で  p1からp0に力が加わるからこっちは正
    spring->p1->f -= (size_f * dir_f);　//向きは逆
  }

  // 全ての質点について順番に処理する
  for(int y = 0; y < POINT_NUM; y++) {
    for(int x = 0; x < POINT_NUM; x++) {
      // cloth->points[x][y].f に、重力による力を加算する
      cloth->points[x][y].f += Mass * gravity;

      // cloth->points[x][y].f に、空気抵抗による力を加算する
      // 空気抵抗による力は速度に定数Dkをかけたもの。ただし向きは速度と逆向き
      // 速度は cloth->points[x][y].v で表される
      cloth->points[x][y].f -= cloth->points[x][y].v * Dk;

    }
  }

  // ここまでで、質点に加わる力をすべて計算し終わった

  // 全ての質点について順番に処理する
  for(int y = 0; y < POINT_NUM; y++) {
    for(int x = 0; x < POINT_NUM; x++) {
      // 頂点が固定されている場合は何もしない
      if(cloth->points[x][y].bFixed) continue;

      Vector3d point_a = cloth->points[x][y].f / Mass;

      cloth->points[x][y].v += point_a * dT;

      cloth->points[x][y].p += cloth->points[x][y].v * dT;

      // オプション. 球体の内部に入るようなら、強制的に外に移動させる
      Vector3d center(0, 0, -POINT_NUM / 2); //球の中心
      Vector3d radius = cloth->points[x][y].p - center;
      double size_r = radius.length();
      radius.normalize();
      if(size_r < 4){
	cloth->points[x][y].p = center + radius * 4;
	cloth->points[x][y].v.set(0,0,0);
      }
    }
  }
}

void motion(int x, int y) {
	int diffX = x - preMousePositionX;
	int diffY = y - preMousePositionY;

	rotateAngleH_deg += diffX * 0.1;
	rotateAngleV_deg += diffY * 0.1;

	preMousePositionX = x;
	preMousePositionY = y;
	glutPostRedisplay();
}

// 一定時間ごとに実行される
void timer(int value) {
	if(bRunning) {
		updateCloth();
		glutPostRedisplay();
	}

	glutTimerFunc(10 , timer , 0);
}

void init(void) {
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

int main(int argc, char *argv[]) {

	bRunning = true;
	cloth = new Cloth();

	glutInit(&argc, argv);
	glutInitWindowSize(600,600);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow(argv[0]);
	glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutTimerFunc(10 , timer , 0);

	init();
	glutMainLoop();
	return 0;
}
