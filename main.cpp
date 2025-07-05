#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint texturaFundo;
GLuint texturaFundoJogo;

GLuint carregarTextura(const char* caminho) {
    std::cout << "Tentando carregar textura: " << caminho << std::endl;
    int largura, altura, canais;
    unsigned char* imagem = stbi_load(caminho, &largura, &altura, &canais, 0);
    if (!imagem) {
        std::cerr << "Erro ao carregar imagem: " << caminho << std::endl;
        std::cerr << "Motivo: " << stbi_failure_reason() << std::endl;
        return 0;
    }

    std::cout << "Imagem carregada com sucesso: " << largura << "x" << altura << " canais: " << canais << std::endl;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    
    GLenum formato = GL_RGB;
    if (canais == 4) formato = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, formato, largura, altura, 0, formato, GL_UNSIGNED_BYTE, imagem);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(imagem);
    std::cout << "Textura criada com ID: " << texID << std::endl;
    return texID;
}

void desenharFundoJogo() {
    if (texturaFundoJogo == 0) return;
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaFundoJogo);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(800.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(800.0f, 600.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 600.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

const float RAIO_EXTERNO = 1.0f;
const float DISTANCIA_ALVO = 10.0f;

float mira_x = 0.0f;
float mira_y = 0.0f;

float pos_dardo[3] = {0.0f, 0.0f, 0.0f};
float pos_final[3] = {0.0f, 0.0f, DISTANCIA_ALVO};
float passo_animacao = 0.0f;
bool lancando = false;

float angulo_braco = 0.0f;
bool animando_lancamento = false;

bool oscilando = true;
float posicao_medidor = 0.0f;
float precisao = 1.0f;

const float GRAVIDADE = 2.0f;

enum State { MENU, GAME, FINAL };
static State state = MENU;
static int mode = 1;

struct Jogador { 
    std::string nome; 
    int pontos, bull, lances; 
    float somaPrec;
};
static std::vector<Jogador> jogadores;
static int atual=0, rodada=1;
const int MAX_LANCES = 5;

static bool resetando = false;

static void drawText2D(int x, int y, const char *s, void *fonte = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2i(x, y);
    while (*s) glutBitmapCharacter(fonte, *s++);
}
static void drawHUD();

void desenharAlvo() {
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, DISTANCIA_ALVO);
    glScalef(1.5f, 1.5f, 1.0f);
	
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.0f, -0.01f);
        for (int i = 0; i <= 100; i++) {
            float angle = 2.0f * 3.14159f * i / 100;
            float x = cos(angle) * RAIO_EXTERNO * 1.05f;
            float y = sin(angle) * RAIO_EXTERNO * 1.05f;
            glVertex3f(x, y, -0.01f);
        }
    glEnd();

    glColor3f(1.0f, 0.0f, 0.0f);
    glutSolidTorus(0.05f, RAIO_EXTERNO, 20, 20);

    glColor3f(1.0f, 1.0f, 1.0f);
    glutSolidTorus(0.05f, 0.7f, 20, 20);

    glColor3f(1.0f, 0.0f, 0.0f);
    glutSolidTorus(0.05f, 0.4f, 20, 20);

    glColor3f(0.0f, 1.0f, 0.0f);
    glutSolidSphere(0.1f, 20, 20);

    glPopMatrix();
}

void desenharMira() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glLineWidth(2.0f);
    glColor3f(0.0f, 1.0f, 0.0f);

    float radius = 0.03f;
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 32; i++) {
        float angle = 2 * 3.14159f * i / 32;
        glVertex2f(mira_x + cos(angle) * radius, mira_y + sin(angle) * radius);
    }
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(mira_x - 0.05f, mira_y);
    glVertex2f(mira_x + 0.05f, mira_y);
    glVertex2f(mira_x, mira_y - 0.05f);
    glVertex2f(mira_x, mira_y + 0.05f);
    glEnd();

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void desenharMedidor() {
	glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    float top = 0.8f;
    float bottom = -0.8f;
    float left = -0.95f;
    float right = -0.85f;

    glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(left, top);
        glVertex2f(right, top);

        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex2f(right, 0.0f);
        glVertex2f(left, 0.0f);

        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex2f(left, 0.0f);
        glVertex2f(right, 0.0f);

        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
    glEnd();

    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINES);
        glVertex2f((left + right) / 2.0f, bottom);
        glVertex2f((left + right) / 2.0f, top);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    float y = posicao_medidor * 0.8f;
    glBegin(GL_TRIANGLES);
        glVertex2f(left - 0.01f, y);
        glVertex2f(right + 0.01f, y + 0.03f);
        glVertex2f(right + 0.01f, y - 0.03f);
    glEnd();

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void desenharMaoMelhorada() {  
    GLUquadric* quad = gluNewQuadric();  
  
    glPushMatrix();  
    glTranslatef(-1.0f, -1.0f, 2.0f);  
    glRotatef(angulo_braco, 0, 0, 1);  
  
    // Palma da mão - esfera maior  
    glColor3f(0.8f, 0.6f, 0.4f);  
    glutSolidSphere(0.15f, 20, 20);  
  
    // Dedos - 4 cilindros com esferas nas juntas  
    float dedoComprimento = 0.2f;  
    float dedoRaio = 0.03f;  
    float espacamento = 0.07f;  
  
    // Dedos do indicador ao mindinho (da direita para esquerda na mão direita)  
    for (int i = 2; i >= -1; --i) {  
        glPushMatrix();  
        glTranslatef(i * espacamento, 0.1f, 0.0f);  
        glRotatef(-30.0f, 1, 0, 0);  
  
        gluQuadricNormals(quad, GLU_SMOOTH);  
        gluCylinder(quad, dedoRaio, dedoRaio, dedoComprimento, 12, 3);  
  
        glTranslatef(0.0f, 0.0f, dedoComprimento);  
        glutSolidSphere(dedoRaio * 1.2f, 12, 12);  
  
        glPopMatrix();  
    }  
  
    // Polegar - cilindro e esfera à esquerda da palma  
    glPushMatrix();  
    glTranslatef(-0.18f, 0.0f, 0.05f);  
    glRotatef(45.0f, 0, 0, 1);  
    glRotatef(-20.0f, 1, 0, 0);  
  
    gluCylinder(quad, dedoRaio, dedoRaio, dedoComprimento * 0.8f, 12, 3);  
    glTranslatef(0.0f, 0.0f, dedoComprimento * 0.8f);  
    glutSolidSphere(dedoRaio * 1.2f, 12, 12);  
    glPopMatrix();  
  
    // Dardo na mão (modelo detalhado)  
    if (!lancando && !animando_lancamento) {  
        glPushMatrix();  
        glColor3f(0.0f, 0.0f, 1.0f);  
        glTranslatef(0.0f, 0.2f, 0.0f);  
        glRotatef(-90, 1, 0, 0);  
        glutSolidCone(0.025f, 0.4f, 20, 1);  
  
        glColor3f(1.0f, 0.0f, 0.0f);  
        glTranslatef(0.0f, 0.0f, 0.4f);  
        glutSolidCone(0.05f, 0.1f, 20, 1);  
  
        glColor3f(1.0f, 1.0f, 0.0f);  
        for (int i = 0; i < 3; ++i) {  
            glPushMatrix();  
            glRotatef(i * 120.0f, 0, 0, 1);  
            glTranslatef(0.0f, 0.03f, 0.0f);  
            glScalef(0.01f, 0.06f, 0.001f);  
            glutSolidCube(1.0f);  
            glPopMatrix();  
        }  
        glPopMatrix();  
    }  
  
    gluDeleteQuadric(quad);  
    glPopMatrix();  
}



void desenharDardo() {
    if (lancando || passo_animacao >= 1.0f) {
        glPushMatrix();
        glTranslatef(pos_dardo[0], pos_dardo[1], pos_dardo[2]);

        glColor3f(0.0f, 0.0f, 1.0f);
        glPushMatrix();
        glRotatef(-90, 1, 0, 0);
        glutSolidCone(0.025f, 0.4f, 20, 1);
        glPopMatrix();

        glColor3f(1.0f, 0.0f, 0.0f);
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.4f);
        glRotatef(-90, 1, 0, 0);
        glutSolidCone(0.05f, 0.1f, 20, 1);
        glPopMatrix();

        glColor3f(1.0f, 1.0f, 0.0f);
        for (int i = 0; i < 3; ++i) {
            glPushMatrix();
            glTranslatef(0.0f, 0.0f, -0.05f);
            glRotatef(i * 120.0f, 0, 0, 1);
            glTranslatef(0.0f, 0.03f, 0.0f);
            glScalef(0.01f, 0.06f, 0.001f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }

        glPopMatrix();
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (state == MENU) {
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0, 800, 0, 600);
        glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texturaFundo);
        glColor3f(1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(800.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(800.0f, 600.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 600.0f);
        glEnd();

        glDisable(GL_TEXTURE_2D);

        glColor3f(1,1,1);
        drawText2D(300, 400, "Jogo de Dardos 3D", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText2D(300, 350, "1 - Jogo 1v1 (2 jogadores)", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText2D(300, 320, "2 - Singleplayer (treino)", GLUT_BITMAP_TIMES_ROMAN_24);

        glEnable(GL_DEPTH_TEST);
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
        glutSwapBuffers();
        return;
    }

    if(state==FINAL){
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0,800,0,600);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glColor3f(1,1,1);
        drawText2D(300,550,"Estatisticas Finais:");
        int y=510;
        for(size_t i=0;i<jogadores.size();++i){
            Jogador &J = jogadores[i];
            std::ostringstream ss;
            ss<<J.nome<<": "<<J.pontos<<" pts, Bulls="<<J.bull
              <<", Lanc="<<J.lances<<", Prec. media="<< (J.somaPrec/J.lances)<<"%";
            drawText2D(100,y,ss.str().c_str());
            y-=30;
        }
        drawText2D(250,50,"Pressione R para reiniciar");
        glEnable(GL_DEPTH_TEST);
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
        glutSwapBuffers();
        return;
    }

    desenharFundoJogo();
    
    drawHUD();
    glLoadIdentity();
    gluLookAt(0,0,0, 0,0,1, 0,1,0);
    desenharAlvo();
    desenharMaoMelhorada();
    desenharDardo();
    desenharMira();
    desenharMedidor();
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (state == MENU) {
        if (key == '1' || key == '2') {
            mode = (key == '1' ? 1 : 2);
            jogadores.clear();
            int n = (mode == 1 ? 2 : 1);
            for (int i = 0; i < n; ++i) {
                Jogador J;
                std::ostringstream os;
                os << "Jogador " << (i + 1);
                J.nome     = os.str();
                J.pontos   = 0;
                J.bull     = 0;
                J.lances   = 0;
                J.somaPrec = 0.0f;
                jogadores.push_back(J);
            }
            atual = 0;
            rodada = 1;
            state = GAME;
            glutPostRedisplay();
        }
        return;
    }
    if (key == ' ' && !animando_lancamento) {
        oscilando = false;
        animando_lancamento = true;
        precisao = 1.0f - fabs(posicao_medidor);
        std::cout << "Precisao capturada: " << precisao * 100 << "%" << std::endl;
    } else if (key == 'r') {
        animando_lancamento = lancando = resetando = false;
        oscilando = true;
        angulo_braco = passo_animacao = posicao_medidor = 0.0f;
        pos_dardo[0] = pos_dardo[1] = pos_dardo[2] = 0.0f;
        if(state==FINAL) state=MENU;
        glutPostRedisplay();
    }
}

void atualizar(int value) {  
    if (oscilando) {  
        posicao_medidor = sin(glutGet(GLUT_ELAPSED_TIME) * 0.005f);  
    }  
  
    if (animando_lancamento) {  
        angulo_braco += 5.0f;  
        if (angulo_braco >= 90.0f) {  
            angulo_braco = 90.0f;  
            animando_lancamento = false;  
            lancando = true;  
            passo_animacao = 0.0f;  
  
            float scatter = (1.0f - precisao) * RAIO_EXTERNO;  
            float offX = ((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * scatter;  
            float offY = - ((rand() / (float)RAND_MAX) * scatter);  
            pos_final[0] = mira_x * RAIO_EXTERNO + offX;  
            pos_final[1] = mira_y * RAIO_EXTERNO + offY;  
            pos_final[2] = DISTANCIA_ALVO;  
        }  
    }  
    else if (lancando) {  
        passo_animacao += 0.02f;  
        if (passo_animacao >= 1.0f) {  
            passo_animacao = 1.0f;  
            lancando = false;  
            resetando = true;  
        }  
        float t = passo_animacao;  
        pos_dardo[0] = pos_final[0] * t;  
        pos_dardo[1] = pos_final[1] * t - 0.5f * GRAVIDADE * t * t;  
        pos_dardo[2] = DISTANCIA_ALVO * t;  
    }  
    else if (resetando) {  
        angulo_braco -= 2.0f;  
        if (angulo_braco <= 0.0f) {  
            angulo_braco    = 0.0f;  
            resetando       = false;  
            oscilando       = true;  
            passo_animacao  = 0.0f;  
            posicao_medidor = 0.0f;  
  
            Jogador &J = jogadores[atual];  
            float dist = sqrt(pos_dardo[0]*pos_dardo[0] + pos_dardo[1]*pos_dardo[1]);  
            int pts = 0;  
            if (dist <= 0.1f) {  
                pts = 50;  
                J.bull++;  
            } else {  
                pts = int((1.0f - dist/RAIO_EXTERNO) * 10.0f);  
                if (pts < 0) pts = 0;  
            }  
            J.pontos   += pts;  
            J.lances   += 1;  
            J.somaPrec += precisao * 100.0f;  
  
            atual++;  
            if (atual >= (int)jogadores.size()) {  
                atual = 0;  
                rodada++;  
                if (rodada > MAX_LANCES) state = FINAL;  
            }  
        }  
    }  
  
    glutPostRedisplay();  
    glutTimerFunc(16, atualizar, 0);  
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    
	stbi_set_flip_vertically_on_load(true);
    
    std::cout << "Carregando texturas..." << std::endl;
    texturaFundoJogo = carregarTextura("imagem-de-fundo-jogo.jpg");
    
    glEnable(GL_TEXTURE_2D);
    texturaFundo = carregarTextura("dartboard.jpg");
    
    std::cout << "Inicialização completa." << std::endl;
}

void mouseMotion(int x, int y) {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    mira_x = (2.0f * x) / w - 1.0f;
    mira_y = 1.0f - (2.0f * y) / h;
    glutPostRedisplay();
}

static void drawHUD(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,800,0,600);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glColor3f(1,1,1);
    for(size_t i=0;i<jogadores.size();++i){
        Jogador &J = jogadores[i];
        std::ostringstream ss;
        ss << J.nome << ": " << J.pontos << " pts, Lanc. "
           << J.lances << "/" << MAX_LANCES;
        drawText2D(10, 580 - 20*i, ss.str().c_str());
    }
    std::ostringstream os;
    os << "Rodada " << rodada;
    drawText2D(600, 580, os.str().c_str());
    glEnable(GL_DEPTH_TEST);
    glPopMatrix(); glMatrixMode(GL_PROJECTION);
    glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Jogo de Dardos 3D - Layout Melhorado");

    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouseMotion);
    glutTimerFunc(0, atualizar, 0);

    glutMainLoop();

    return 0;
}