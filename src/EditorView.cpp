#include "EditorView.h"

// TODO: el -50 de la inicializacion de la camara tiene que ver con el marginXoffset
EditorView::EditorView(
    const sf::RenderWindow &window,
    const sf::String &workingDirectory,
    EditorContent &editorContent,
    bool verticalMode)
    : verticalMode(verticalMode), content(editorContent),
      camera(sf::FloatRect(0, 0, window.getSize().x, window.getSize().y)),
      deltaScroll(20), deltaRotation(2), deltaZoomIn(0.8f), deltaZoomOut(1.2f) {

    // this->font.loadFromFile("fonts/FreeMono.ttf");
    this->font.loadFromFile(workingDirectory + "fonts/DejaVuSansMono.ttf");

    this->setFontSize(18);  // Important to call

    if (this->verticalMode) {
        this->bottomLimitPx = 1;
        this->rightLimitPx = this->content.linesCount() * this->fontSize;
    } else {
        this->bottomLimitPx = this->content.linesCount() * this->fontSize;
        this->rightLimitPx = 1;
    }

    // TODO: Cambiarlo en relacion a la fontsize
    this->marginXOffset = 45;
    this->colorMargin = sf::Color(32, 44, 68);

    this->colorChar = sf::Color::White;
    this->colorSelection = sf::Color(106, 154, 232);
    
    if (this->verticalMode) {
        this->colorMargin = sf::Color(0, 59, 0);
        this->colorChar = sf::Color(0, 255, 65);
        this->colorSelection = sf::Color(0, 143, 17);
    }
}

// TODO: Divide fontsize from lineheight
void EditorView::setFontSize(int fontSize) {
    this->fontSize = fontSize;
    this->lineHeight = fontSize;

    // HACK: Because I use only monospace fonts, every char is the same width
    //       so I get the width drawing a single character (A WIDE ONE TO BE SURE)
    sf::Text tmpText;
    tmpText.setFont(this->font);
    tmpText.setCharacterSize(this->fontSize);
    tmpText.setString("_");
    float textwidth = tmpText.getLocalBounds().width;
    this->charWidth = textwidth;
}

float EditorView::getRightLimitPx() {
    return this->rightLimitPx;
}

float EditorView::getBottomLimitPx() {
    return this->bottomLimitPx;
}

int EditorView::getLineHeight() {
    return this->lineHeight;
}

int EditorView::getCharWidth() {
    return this->charWidth;
}

void EditorView::draw(sf::RenderWindow &window) {
    // Draw in vertical mode
    if (this->verticalMode) {
        this->drawVertical(window);
        return;
    }

    // Draw in normal (horizontal) mode
    // TODO: El content devuelve un vector diciendo que alto tiene cada linea,
    //      por ahora asumo que todas miden "1" de alto
    this->drawLines(window);

    // Dibujo los numeros de la izquierda

    // TODO: Hacer una clase separada para el margin
    for (int lineNumber = 1; lineNumber <= this->content.linesCount(); lineNumber++) {
        int lineHeight = 1;

        int blockHeight = lineHeight * this->fontSize;

        sf::Text lineNumberText;
        lineNumberText.setFillColor(this->colorChar);
        lineNumberText.setFont(this->font);
        lineNumberText.setString(std::to_string(lineNumber));
        lineNumberText.setCharacterSize(this->fontSize - 1);
        lineNumberText.setPosition(0, blockHeight * (lineNumber - 1));

        sf::RectangleShape marginRect(sf::Vector2f(this->marginXOffset - 5, blockHeight));
        marginRect.setFillColor(this->colorMargin);
        marginRect.setPosition(0, blockHeight * (lineNumber - 1));

        window.draw(marginRect);
        window.draw(lineNumberText);
    }

    this->drawCursor(window);
}

void EditorView::drawVertical(sf::RenderWindow &window) {
    // TODO: El content devuelve un vector diciendo que alto tiene cada linea,
    //      por ahora asumo que todas miden "1" de alto
    // TODO: The content returns a vector saying what height each line has,
    //      for now I assume that everything measures "1" in height (has height 1).
    //      Por ejemplo, una línea puede tener alto mayor que "1" si pasa el tamaño de la ventana
    this->drawLinesVertical(window);

    // Dibujo los numeros de la izquierda
    // Draw the numbers on the left
    float width = std::max(this->rightLimitPx, this->getWidth());

    // TODO: Hacer una clase separada para el margin
    // TODO: Make a separate class for the margin
    for (int lineNumber = 1; lineNumber <= this->content.linesCount(); lineNumber++) {
        int lineHeight = 1;

        int blockHeight = lineHeight * this->fontSize;

        sf::Text lineNumberText;
        lineNumberText.setFillColor(this->colorChar);
        lineNumberText.setFont(this->font);
        lineNumberText.setString(std::to_string(lineNumber));
        lineNumberText.setCharacterSize(this->fontSize - 1);
        lineNumberText.setPosition(width - blockHeight * (lineNumber - 1), 0);
        lineNumberText.setRotation(90);

        sf::RectangleShape marginRect(sf::Vector2f(blockHeight, this->marginXOffset - 5));
        marginRect.setFillColor(this->colorMargin);
        marginRect.setPosition(width - blockHeight * lineNumber, 0);

        window.draw(marginRect);
        window.draw(lineNumberText);
    }

    this->drawCursorVertical(window);
}

// TODO: esto lo deberia manejar el editorContetn de alguna forma? 4 harcodeado
int colsOf(sf::String &currentLineText) {
    int cols = 0;
    for (char c : currentLineText) {
        if (c == '\t') {
            cols += 4;
        } else {
            cols++;
        }
    }
    return cols;
}

// TODO: Reemplazar fontSize por fontHeight especifica para cada tipo de font.
// TODO: Multiples cursores similar a Selecciones, que los moveUp.. etc muevan todos
// TODO: Que devuelva un vector diciendo el alto que ocupa el dibujo de cada linea, para saber el tamaño de cada linea en el margen
void EditorView::drawLines(sf::RenderWindow &window) {
    this->bottomLimitPx = this->content.linesCount() * this->fontSize;

    for (int lineNumber = 0; lineNumber < this->content.linesCount(); lineNumber++) {
        sf::String line = this->content.getLine(lineNumber);
        sf::String currentLineText = "";

        // TODO: Esto es al pe?
        this->rightLimitPx = std::max((int)this->rightLimitPx, (int)(this->charWidth * line.getSize()));

        float offsetx = this->marginXOffset;
        bool previousSelected = false;

        for (int charIndexInLine = 0; charIndexInLine <= (int)line.getSize(); charIndexInLine++) {
            // En general hay una unica seleccion, en el futuro podria haber mas de una
            bool currentSelected = content.isSelected(lineNumber, charIndexInLine);

            // Cuando hay un cambio, dibujo el tipo de seleccion anterior
            // Tambien dibujo cuando es el fin de la linea actual
            if (currentSelected != previousSelected || charIndexInLine == (int)line.getSize()) {
                sf::Text texto;
                texto.setFillColor(this->colorChar);
                texto.setFont(font);
                texto.setString(currentLineText);
                texto.setCharacterSize(this->fontSize);
                texto.setPosition(offsetx, lineNumber * this->fontSize);

                if (previousSelected) {
                    int currentColsAmount = colsOf(currentLineText);
                    sf::RectangleShape selectionRect(
                        sf::Vector2f(this->charWidth * currentColsAmount, this->fontSize));
                    selectionRect.setFillColor(this->colorSelection);
                    // TODO: Que el +2 no sea un numero magico
                    selectionRect.setPosition(offsetx, 2 + lineNumber * this->fontSize);
                    window.draw(selectionRect);
                }

                window.draw(texto);

                previousSelected = currentSelected;
                offsetx += this->charWidth * colsOf(currentLineText);
                currentLineText = "";
            }

            // Voy acumulando la string de la linea actual
            currentLineText += line[charIndexInLine];
        }
    }
}

void EditorView::drawLinesVertical(sf::RenderWindow &window) {
    this->rightLimitPx = this->content.linesCount() * this->fontSize;
    float width = std::max(this->rightLimitPx, this->getWidth());

    for (int lineNumber = 0; lineNumber < this->content.linesCount(); lineNumber++) {
        sf::String line = this->content.getLine(lineNumber);

        // TODO: Esto es al pe?
        this->bottomLimitPx = std::max((int)this->bottomLimitPx, (int)(this->fontSize * line.getSize()));

        float offsety = 0;

        for (int charIndexInLine = 0; charIndexInLine < (int)line.getSize(); charIndexInLine++) {
            sf::String currentLineText = line[charIndexInLine];

            // En general hay una unica seleccion, en el futuro podria haber mas de una
            // In general there is a unique selection, in the future there could be more than one
            bool currentSelected = content.isSelected(lineNumber, charIndexInLine);

            sf::Text texto;
            texto.setFillColor(this->colorChar);
            texto.setFont(font);
            texto.setString(currentLineText);
            texto.setCharacterSize(this->fontSize);
            texto.setPosition(width - (lineNumber + 1) * this->fontSize, offsety + this->marginXOffset);

            if (currentSelected) {
                sf::RectangleShape selectionRect(
                    sf::Vector2f(this->fontSize, this->fontSize));
                selectionRect.setFillColor(this->colorSelection);
                // TODO: Que el +2 no sea un numero magico
                // TODO: Make the +2 not a magic number
                selectionRect.setPosition(width - 2 - (lineNumber + 1) * this->fontSize, offsety + this->marginXOffset);
                window.draw(selectionRect);
            }

            window.draw(texto);

            offsety += this->fontSize;
        }
    }
}

// TODO: No harcodear constantes aca. CursorView?
void EditorView::drawCursor(sf::RenderWindow &window) {
    int offsetY = 2;
    int cursorDrawWidth = 2;

    int charWidth = getCharWidth();
    int lineHeight = getLineHeight();

    std::pair<int, int> cursorPos = this->content.cursorPosition();
    int lineN = cursorPos.first;
    int column = cursorPos.second;

    sf::RectangleShape cursorRect(sf::Vector2f(cursorDrawWidth, lineHeight));
    cursorRect.setFillColor(this->colorChar);

    cursorRect.setPosition(
        this->marginXOffset + column * charWidth,
        (lineN * lineHeight) + offsetY);

    window.draw(cursorRect);
}

void EditorView::drawCursorVertical(sf::RenderWindow &window) {
    float width = std::max(this->rightLimitPx, this->getWidth());
    int offsetY = this->marginXOffset;
    int cursorDrawWidth = 2;

    int lineHeight = getLineHeight();

    std::pair<int, int> cursorPos = this->content.cursorPosition();
    int lineN = cursorPos.first;
    int column = cursorPos.second;

    sf::RectangleShape cursorRect(sf::Vector2f(lineHeight, cursorDrawWidth));
    cursorRect.setFillColor(this->colorChar);

    cursorRect.setPosition(
        width - (lineN + 1) * lineHeight,
        column * lineHeight + offsetY);

    window.draw(cursorRect);
}

// TODO: Esto no considera que los tabs \t existen
// Asume que el x=0 es donde empieza el texto
std::pair<int, int> EditorView::getDocumentCoords(
    float mouseX, float mouseY) {
    
    if (this->verticalMode) {
        return this->getDocumentCoordsVertical(mouseX, mouseY);
    }

    mouseX -= this->marginXOffset;
    int lineN = mouseY / this->getLineHeight();
    int charN = 0;

    // Restrinjo numero de linea a la altura del documento
    int lastLine = this->content.linesCount() - 1;

    if (lineN < 0) {
        lineN = 0;
        charN = 0;
    } else if (lineN > lastLine) {
        lineN = lastLine;
        charN = this->content.colsInLine(lineN);
    } else {
        lineN = std::max(lineN, 0);
        lineN = std::min(lineN, lastLine);

        // column != charN because tabs
        int column = std::round(mouseX / this->getCharWidth());
        charN = this->content.getCharIndexOfColumn(lineN, column);

        // Restrinjo numero de caracter a cant de caracteres de la linea
        charN = std::max(charN, 0);
        charN = std::min(charN, this->content.colsInLine(lineN));
    }

    return std::pair<int, int>(lineN, charN);
}

std::pair<int, int> EditorView::getDocumentCoordsVertical(
    float mouseX, float mouseY) {

    mouseY -= this->marginXOffset;

    int lineN = (std::max(this->rightLimitPx, this->getWidth()) - mouseX) / this->getLineHeight();
    int charN = 0;

    // Restrinjo numero de linea a la altura del documento
    // Restrict line number to the height of the document
    int lastLine = this->content.linesCount() - 1;

    if (lineN < 0) {
        lineN = 0;
        charN = 0;
    } else if (lineN > lastLine) {
        lineN = lastLine;
        charN = this->content.colsInLine(lineN);
    } else {
        lineN = std::max(lineN, 0);
        lineN = std::min(lineN, lastLine);

        // column != charN because tabs
        int column = std::round(mouseY / this->getLineHeight());
        charN = this->content.getCharIndexOfColumn(lineN, column);

        // Restrinjo numero de caracter a cant de caracteres de la linea
        charN = std::max(charN, 0);
        charN = std::min(charN, this->content.colsInLine(lineN));
    }

    return std::pair<int, int>(lineN, charN);
}

void EditorView::scrollUp(sf::RenderWindow &window) {
    float height = this->getHeight();
    auto camPos = this->camera.getCenter();
    // Scrolleo arriba solo si no me paso del limite superior
    if (camPos.y - height / 2 > 0) {
        this->camera.move(0, -this->deltaScroll);
    }
}

void EditorView::scrollDown(sf::RenderWindow &window) {
    float height = this->getHeight();
    float bottomLimit = std::max(this->getBottomLimitPx() + 20, height);
    if (this->verticalMode) {
        bottomLimit = std::max(this->bottomLimitPx + this->marginXOffset + 20, height);
    }
    auto camPos = this->camera.getCenter();
    // Numero magico 20 como un plus
    if (camPos.y + height / 2 < bottomLimit) {
        this->camera.move(0, this->deltaScroll);
    }
}

void EditorView::scrollLeft(sf::RenderWindow &window) {
    float width = this->getWidth();
    float leftLimit = 0;
    if (this->verticalMode) {
        leftLimit = std::max(float(-20), width - this->rightLimitPx);
    }
    auto camPos = this->camera.getCenter();
    // Scrolleo arriba si no me paso del limite izquierdo
    if (camPos.x - width / 2 > leftLimit) {
        this->camera.move(-this->deltaScroll, 0);
    }
}

void EditorView::scrollRight(sf::RenderWindow &window) {
    float width = this->getWidth();
    float rightLimit = std::max(this->rightLimitPx + this->marginXOffset, width);
    if (this->verticalMode) {
        rightLimit = std::max(this->rightLimitPx, width);
    }
    auto camPos = this->camera.getCenter();
    // Numero magico 20 como un plus
    if (camPos.x + width / 2 < rightLimit) {
        this->camera.move(this->deltaScroll, 0);
    }
}

void EditorView::rotateLeft() {
    this->camera.rotate(this->deltaRotation);
}

void EditorView::rotateRight() {
    this->camera.rotate(-this->deltaRotation);
}

void EditorView::zoomIn() {
    this->camera.zoom(this->deltaZoomIn);
}

void EditorView::zoomOut() {
    this->camera.zoom(this->deltaZoomOut);
}

void EditorView::setCameraBounds(int width, int height) {
    if (this->verticalMode) {
        this->camera = sf::View(
            sf::FloatRect(std::max(this->rightLimitPx, float(width)) - width, 0, width, height));
    } else {
        this->camera = sf::View(sf::FloatRect(0, 0, width, height));
    }
}

float EditorView::getWidth() {
    return this->camera.getSize().x;
}

float EditorView::getHeight() {
    return this->camera.getSize().y;
}

sf::View EditorView::getCameraView() {
    return this->camera;
}
