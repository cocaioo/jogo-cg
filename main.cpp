#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>

// Definicoes necessarias para OpenGL
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// biblioteca para carregamento de imagens
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// variavel para textura do fundo
GLuint texturaFundo;

// textura para fundo do jogo
GLuint texturaFundoJogo;

// Variáveis para animação do GIF no menu
static std::vector<GLuint> texturaFrames;
static int frameAtual = 0;
static int totalFrames = 70; // Ajuste para o número real de frames extraídos

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

// Braco
float angulo_braco = 0.0f;
bool animando_lancamento = false;

// Medidor de precisao
bool oscilando = true;
float posicao_medidor = 0.0f;  // Varia de -1 a 1
float precisao = 1.0f;         // De 0 a 1

// Gravidade ajustada
const float GRAVIDADE = 2.0f;

enum State { MENU, GAME, FINAL };
static State state = MENU;
static int mode = 1;   // 1=1v1,2=2v2

// estatisticas
struct Jogador { 
    std::string nome; 
    int pontos, bull, lances; 
    float somaPrec;
};
static std::vector<Jogador> jogadores;
static int atual=0, rodada=1;
const int MAX_LANCES = 5;

// --- Adicione no topo, apos as flags existentes ---
static bool resetando = false;  // controla animacao de retorno da mao

// funcao para carregar textura (versao compativel)
GLuint carregarTextura(const char* caminho) {
    std::cout << "Tentando carregar textura: " << caminho << std::endl;
    int largura, altura, canais;
    unsigned char* imagem = stbi_load(caminho, &largura, &altura, &canais, 0);
    if (!imagem) {
        std::cerr << "Erro ao carregar imagem: " << caminho << std::endl;
        std::cerr << "Motivo: " << stbi_failure_reason() << std::endl;
        return 0; // Retorna 0 em vez de sair do programa
    }

    std::cout << "Imagem carregada com sucesso: " << largura << "x" << altura << " canais: " << canais << std::endl;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    
    // Usar RGB sempre para compatibilidade
    GLenum formato = GL_RGB;
    if (canais == 4) formato = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, formato, largura, altura, 0, formato, GL_UNSIGNED_BYTE, imagem);
    
    // Usar constantes basicas do OpenGL 1.1
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(imagem);
    std::cout << "Textura criada com ID: " << texID << std::endl;
    return texID;
}

// funcao auxiliar para texto 2D em pixels
static void drawText2D(int x, int y, const char *s, void *fonte = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2i(x, y);
    while (*s) glutBitmapCharacter(fonte, *s++);
}
// prototipo para funcao de HUD (deve vir antes de display)
static void drawHUD();

void desenharFundoJogo() {
    // Verificar se a textura foi carregada corretamente
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
    glColor3f(1.0f, 1.0f, 1.0f); // cor branca para nao alterar a textura

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

void desenharAlvo() {
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, DISTANCIA_ALVO);

    // aumentando o tamanho do alvo
    glScalef(1.5f, 1.5f, 1.0f);
    
    // Fundo solido atras do alvo (plano circular)
    glColor3f(0.1f, 0.1f, 0.1f);  // Cor escura opaca
    glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.0f, -0.01f);  // Centro (ligeiramente atras do plano Z = 0 do alvo)
        for (int i = 0; i <= 100; i++) {
            float angle = 2.0f * 3.14159f * i / 100;
            float x = cos(angle) * RAIO_EXTERNO * 1.05f;
            float y = sin(angle) * RAIO_EXTERNO * 1.05f;
            glVertex3f(x, y, -0.01f);  // Fundo levemente atras
        }
    glEnd();

    // Alvo (em cima do fundo)
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

    // Circulo central
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

    // Barra com degrade vertical (vermelho -> amarelo -> verde)
    float top = 0.8f;
    float bottom = -0.8f;
    float left = -0.95f;
    float right = -0.85f;

    glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 0.0f);  // Vermelho no topo
        glVertex2f(left, top);
        glVertex2f(right, top);

        glColor3f(1.0f, 1.0f, 0.0f);  // Amarelo no meio
        glVertex2f(right, 0.0f);
        glVertex2f(left, 0.0f);

        glColor3f(1.0f, 1.0f, 0.0f);  // Amarelo no meio
        glVertex2f(left, 0.0f);
        glVertex2f(right, 0.0f);

        glColor3f(0.0f, 1.0f, 0.0f);  // Verde na base
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
    glEnd();

    // Linha central da barra
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINES);
        glVertex2f((left + right) / 2.0f, bottom);
        glVertex2f((left + right) / 2.0f, top);
    glEnd();

    // Ponteiro oscilante (triangulo)
    glColor3f(1.0f, 1.0f, 1.0f);  // Branco para contraste
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

void desenharMaoEDardo() {
    glPushMatrix();
    glTranslatef(-1.0f, -1.0f, 2.0f);
    glRotatef(angulo_braco, 0, 0, 1);

    // Braco
    glColor3f(0.8f,0.6f,0.4f);
    glPushMatrix(); glScalef(0.2f,1.0f,0.2f); glutSolidCube(1.0f); glPopMatrix();

    // Mao (esfera)
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

      // Dardo na mao (se nao iniciado)
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

        // Desenhar frame atual do GIF animado
        if (!texturaFrames.empty()) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, texturaFrames[frameAtual]);
            glColor3f(1.0f, 1.0f, 1.0f);

            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(800.0f, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(800.0f, 600.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 600.0f);
            glEnd();

            glDisable(GL_TEXTURE_2D);
        }

        // Desenhar retângulos de fundo para os botões (contraste)
        glColor3f(0.0f, 0.0f, 0.5f); // Azul escuro para o botão 1
        glBegin(GL_QUADS);
            glVertex2f(290, 345);
            glVertex2f(600, 345);
            glVertex2f(600, 375);
            glVertex2f(290, 375);
        glEnd();

        glColor3f(0.0f, 0.0f, 0.5f); // Azul escuro para o botão 2
        glBegin(GL_QUADS);
            glVertex2f(290, 315);
            glVertex2f(600, 315);
            glVertex2f(600, 345);
            glVertex2f(290, 345);
        glEnd();

        // Textos do menu
        glColor3f(1,1,1);
        drawText2D(300, 360, "1 - Jogo 1v1 (2 jogadores)", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText2D(300, 330, "2 - Singleplayer (treino)", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText2D(300, 400, "Jogo de Dardos 3D", GLUT_BITMAP_TIMES_ROMAN_24);

        glEnable(GL_DEPTH_TEST);
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
        glutSwapBuffers();
        return;
    }

    if(state==FINAL){
        // tela de estatasticas finais
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

    // --- codigo existente do jogo ---
    // Desenhar fundo do jogo
    desenharFundoJogo();
    
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
    // oscilacao do medidor
    if (oscilando) {
        posicao_medidor = sin(glutGet(GLUT_ELAPSED_TIME) * 0.005f);
    }

    // animacao do braco subindo
    if (animando_lancamento) {
        angulo_braco += 5.0f;
        if (angulo_braco >= 90.0f) {
            angulo_braco = 90.0f;
            animando_lancamento = false;
            lancando = true;
            passo_animacao = 0.0f;

            // calcula dispersao conforme precisao
            float scatter = (1.0f - precisao) * RAIO_EXTERNO;
            float offX = ((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * scatter;
            float offY = ((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * scatter;
            pos_final[0] = mira_x * RAIO_EXTERNO + offX;
            pos_final[1] = mira_y * RAIO_EXTERNO + offY;
            pos_final[2] = DISTANCIA_ALVO;
        }
    }
    // animacao do dardo voando
    else if (lancando) {
        passo_animacao += 0.02f;
        if (passo_animacao >= 1.0f) {
            passo_animacao = 1.0f;
            lancando = false;
            resetando = true;           // inicia reset da mao e medidor
        }
        float t = passo_animacao;
        pos_dardo[0] = pos_final[0] * t;
        pos_dardo[1] = pos_final[1] * t - 0.5f * GRAVIDADE * t * t;
        pos_dardo[2] = DISTANCIA_ALVO * t;
    }
    // reset: desce o braco e reinicia o medidor
    else if (resetando) {
        angulo_braco -= 2.0f;
        if (angulo_braco <= 0.0f) {
            angulo_braco    = 0.0f;
            resetando       = false;
            oscilando       = true;
            passo_animacao  = 0.0f;
            posicao_medidor = 0.0f;

            // calculo de pontos exemplo
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

            // avanca jogador/rodada
            atual++;
            if (atual >= (int)jogadores.size()) {
                atual = 0;
                rodada++;
                if (rodada > MAX_LANCES) state = FINAL;
            }
        }
    }

    // Atualiza frame do GIF animado no menu
    if (state == MENU && !texturaFrames.empty()) {
        frameAtual = (frameAtual + 1) % (int)texturaFrames.size();
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

    // Carregar frames do GIF animado para o menu
    for (int i = 0; i < totalFrames; ++i) {  
    char filename[64];  
    sprintf(filename, "frames/frame-%d.png", i);  // sem zeros à esquerda  
    GLuint tex = carregarTextura(filename);  
    if (tex != 0) {  
        texturaFrames.push_back(tex);  
    } else {  
        std::cerr << "Falha ao carregar frame: " << filename << std::endl;  
    }  
}

    std::cout << "Inicialização completa." << std::endl;
}

// --- Definir callback que faltava para movimentacao da mira ---
void mouseMotion(int x, int y) {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    mira_x = (2.0f * x) / w - 1.0f;
    mira_y = 1.0f - (2.0f * y) / h;
    glutPostRedisplay();
}

// --- melhor exibicao de estatasticas no jogo ---
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