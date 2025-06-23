#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>

// Constantes do alvo
const float RAIO_EXTERNO = 1.0f;
const float DISTANCIA_ALVO = 10.0f;

// Mira
float mira_x = 0.0f;
float mira_y = 0.0f;

// Dardo
float pos_dardo[3] = {0.0f, 0.0f, 0.0f};
float pos_final[3] = {0.0f, 0.0f, DISTANCIA_ALVO};
float passo_animacao = 0.0f;
bool lancando = false;

// Braço
float angulo_braco = 0.0f;
bool animando_lancamento = false;

// Medidor de precisão
bool oscilando = true;
float posicao_medidor = 0.0f;  // Varia de -1 a 1
float precisao = 1.0f;         // De 0 a 1

// Gravidade ajustada
const float GRAVIDADE = 2.0f;

enum State { MENU, GAME, FINAL };
static State state = MENU;
static int mode = 1;   // 1=1v1,2=2v2
// estatísticas
struct Jogador { 
    std::string nome; 
    int pontos, bull, lances; 
    float somaPrec;
};
static std::vector<Jogador> jogadores;
static int atual=0, rodada=1;
const int MAX_LANCES = 5;
 
// --- Adicione no topo, após as flags existentes ---
static bool resetando = false;  // controla animação de retorno da mão

// função auxiliar para texto 2D em pixels
static void drawText2D(int x, int y, const char *s) {
    glRasterPos2i(x, y);
    while (*s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *s++);
}
// protótipo para função de HUD (deve vir antes de display)
static void drawHUD();

void desenharAlvo() {
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, DISTANCIA_ALVO);

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

    // Círculo central
    float radius = 0.03f;
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 32; i++) {
        float angle = 2 * 3.14159f * i / 32;
        glVertex2f(mira_x + cos(angle) * radius, mira_y + sin(angle) * radius);
    }
    glEnd();

    // Linhas verticais e horizontais
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

    // Caixa de fundo
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-0.95f, -0.8f);
    glVertex2f(-0.85f, -0.8f);
    glVertex2f(-0.85f, 0.8f);
    glVertex2f(-0.95f, 0.8f);
    glEnd();

    // Linha central da barra
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINES);
    glVertex2f(-0.9f, -0.8f);
    glVertex2f(-0.9f, 0.8f);
    glEnd();

    // Ponteiro
    glColor3f(0.0f, 1.0f, 0.0f);
    float y = posicao_medidor * 0.8f;
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.96f, y);
    glVertex2f(-0.84f, y + 0.03f);
    glVertex2f(-0.84f, y - 0.03f);
    glEnd();

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void desenharMaoEDardo() {
    glPushMatrix();
    glTranslatef(-1.0f, -1.0f, 2.0f);
    glRotatef(angulo_braco, 0, 0, 1);

    // Braço
    glColor3f(0.8f,0.6f,0.4f);
    glPushMatrix(); glScalef(0.2f,1.0f,0.2f); glutSolidCube(1.0f); glPopMatrix();

    // Mão (esfera)
    glPushMatrix(); glTranslatef(0.0f,0.5f,0.0f);
      glColor3f(0.8f,0.6f,0.4f); glutSolidSphere(0.15f,20,20);

      // Dedos (caixinhas)
      for (int i = -1; i <= 1; ++i) {
          glPushMatrix();
            glTranslatef(i * 0.04f, 0.62f, 0.05f);
            glRotatef(-30.0f, 1, 0, 0);
            glColor3f(0.8f,0.6f,0.4f);
            glPushMatrix(); glScalef(0.02f,0.12f,0.02f); glutSolidCube(1.0f); glPopMatrix();
          glPopMatrix();
      }

      // Dardo na mão (se não iniciado)
      if (!lancando && !animando_lancamento) {
          glColor3f(0.0f,0.0f,1.0f);
          glPushMatrix();
            glTranslatef(0.0f,0.2f,0.0f);
            glScalef(0.05f,0.4f,0.05f);
            glutSolidCube(1.0f);
          glPopMatrix();
          glColor3f(1.0f,0.0f,0.0f);
          glPushMatrix();
            glTranslatef(0.0f,0.42f,0.0f);
            glutSolidCone(0.05f,0.1f,10,2);
          glPopMatrix();
      }
    glPopMatrix();
    glPopMatrix();
}

void desenharDardo() {
    if (lancando || passo_animacao >= 1.0f) {
        glPushMatrix();
        glTranslatef(pos_dardo[0], pos_dardo[1], pos_dardo[2]);

        // Corpo
        glColor3f(0.0f, 0.0f, 1.0f);
        glPushMatrix();
        glScalef(0.05f, 0.4f, 0.05f);
        glutSolidCube(1.0f);
        glPopMatrix();

        // Ponta
        glColor3f(1.0f, 0.0f, 0.0f);
        glPushMatrix();
        glTranslatef(0.0f, 0.22f, 0.0f);
        glutSolidCone(0.05f, 0.1f, 10, 2);
        glPopMatrix();

        glPopMatrix();
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (state == MENU) {
        // setup ortho 2D
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0, 800, 0, 600);
        glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
        glDisable(GL_DEPTH_TEST);

        glColor3f(1,1,1);
        drawText2D(300, 400, "Jogo de Dardos 3D");
        drawText2D(300, 350, "1 - Jogo 1v1 (2 jogadores)");
        drawText2D(300, 320, "2 - Singleplayer (treino)");

        glEnable(GL_DEPTH_TEST);
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
        glutSwapBuffers();
        return;
    }

    if(state==FINAL){
        // tela de estatísticas finais
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

    // --- código existente do jogo ---
    drawHUD();
    glLoadIdentity();
    gluLookAt(0,0,0, 0,0,1, 0,1,0);
    desenharAlvo();
    desenharMaoEDardo();
    desenharDardo();
    desenharMira();
    desenharMedidor();
    glutSwapBuffers();
}

// --- Captura tecla para menu e jogo ---
void keyboard(unsigned char key, int x, int y) {
    if (state == MENU) {
        if (key == '1' || key == '2') {
            mode = (key == '1' ? 1 : 2);
            jogadores.clear();
            // 1v1 = 2 jogadores, singleplayer = 1 jogador
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
    // --- restante do seu keyboard() ---
    if (key == ' ' && !animando_lancamento) {
        oscilando = false;
        animando_lancamento = true;
        precisao = 1.0f - fabs(posicao_medidor);
        std::cout << "Precisão capturada: " << precisao * 100 << "%" << std::endl;
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
    // oscilação do medidor
    if (oscilando) {
        posicao_medidor = sin(glutGet(GLUT_ELAPSED_TIME) * 0.005f);
    }

    // animação do braço subindo
    if (animando_lancamento) {
        angulo_braco += 5.0f;
        if (angulo_braco >= 90.0f) {
            angulo_braco = 90.0f;
            animando_lancamento = false;
            lancando = true;
            passo_animacao = 0.0f;

            // cálcula dispersão conforme precisão
            float scatter = (1.0f - precisao) * RAIO_EXTERNO;
            float offX = ((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * scatter;
            float offY = ((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * scatter;
            pos_final[0] = mira_x * RAIO_EXTERNO + offX;
            pos_final[1] = mira_y * RAIO_EXTERNO + offY;
            pos_final[2] = DISTANCIA_ALVO;
        }
    }
    // animação do dardo voando
    else if (lancando) {
        passo_animacao += 0.02f;
        if (passo_animacao >= 1.0f) {
            passo_animacao = 1.0f;
            lancando = false;
            resetando = true;           // inicia reset da mão e medidor
        }
        float t = passo_animacao;
        pos_dardo[0] = pos_final[0] * t;
        pos_dardo[1] = pos_final[1] * t - 0.5f * GRAVIDADE * t * t;
        pos_dardo[2] = DISTANCIA_ALVO * t;
    }
    // reset: desce o braço e reinicia o medidor
    else if (resetando) {
        angulo_braco -= 2.0f;
        if (angulo_braco <= 0.0f) {
            angulo_braco    = 0.0f;
            resetando       = false;
            oscilando       = true;
            passo_animacao  = 0.0f;
            posicao_medidor = 0.0f;

            // cálculo de pontos exemplo
            Jogador &J = jogadores[atual];
            float dist = sqrt(pos_dardo[0]*pos_dardo[0] +
                              pos_dardo[1]*pos_dardo[1]);
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

            // avança jogador/rodada
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
}

// --- Definir callback que faltava para movimentação da mira ---
void mouseMotion(int x, int y) {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    mira_x = (2.0f * x) / w - 1.0f;
    mira_y = 1.0f - (2.0f * y) / h;
    glutPostRedisplay();
}

// --- melhor exibição de estatísticas no jogo ---
static void drawHUD(){
    // mostra placar e lances restantes
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
    // Rodada
    {
      std::ostringstream os;
      os << "Rodada " << rodada;
      drawText2D(600, 580, os.str().c_str());
    }
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
