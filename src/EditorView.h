#ifndef TextView_H
#define TextView_H

#include <SFML/Graphics.hpp>
#include <cmath>
#include "EditorContent.h"

class EditorView {
   public:
    EditorView(const sf::RenderWindow &window,
        const sf::String &workingDirectory,
        EditorContent &editorContent,
        bool verticalMode=false);

    void draw(sf::RenderWindow &window);
    void setFontSize(int fontSize);

    void scrollUp(sf::RenderWindow &window);
    void scrollDown(sf::RenderWindow &window);
    void scrollLeft(sf::RenderWindow &window);
    void scrollRight(sf::RenderWindow &window);

    void scrollTo(float x, float y);

    void rotateLeft();
    void rotateRight();

    void zoomIn();
    void zoomOut();

    int getLineHeight();
    int getCharWidth();

    float getRightLimitPx();
    float getBottomLimitPx();

    sf::View getCameraView();
    void setCameraBounds(int width, int height);

    void setDeltaScroll(float delta);
    void setDeltaRotation(float delta);

    // TODO: Replace std::pair with coordinates object
    std::pair<int,int> getDocumentCoords(float mouseX, float mouseY);

   private:
    EditorContent &content;

    void drawVertical(sf::RenderWindow &window);
    void drawLines(sf::RenderWindow &window);
    void drawLinesVertical(sf::RenderWindow &window);
    void drawCursor(sf::RenderWindow &window);
    void drawCursorVertical(sf::RenderWindow &window);

    bool verticalMode;

    sf::Font font;
    int fontSize;
    int marginXOffset;
    sf::Color colorMargin;

    int lineHeight;
    int charWidth;

    float rightLimitPx;
    float bottomLimitPx;

    sf::Color colorChar;
    sf::Color colorSelection;

    sf::View camera;
    float getWidth();
    float getHeight();
    float deltaScroll;
    float deltaRotation;
    float deltaZoomIn, deltaZoomOut;

    std::pair<int,int> getDocumentCoordsVertical(float mouseX, float mouseY);
};

#endif
