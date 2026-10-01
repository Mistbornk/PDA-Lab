#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <cmath>
#include <set>
#include <algorithm>
#include "PrettyTable.cpp"
using namespace std;


struct Rectangle {
    double x, y, width, height;
    Rectangle(vector<double>& in){
        x = in[0]; y = in[1];
        width = in[2]; height = in[3];
    }
};


struct Coor{
    double x;
    double y;
    Coor(double x_in, double y_in){
        x = x_in;
        y = y_in;
    }
    Coor(): x(0), y(0){};
};

class Cell{
public:
    Cell() {}
    ~Cell() {}

    void setName(const std::string& name) {
        this->name = name;
    }

    std::string getName() const {
        return name;
    }

    void setOriCoor(const Coor& coor) {
        this->oriCoor = coor;
    }

    Coor getOriCoor() const {
        return oriCoor;
    }

    void setNewCoor(const Coor& coor) {
        this->newCoor = coor;
    }

    Coor getNewCoor() const {
        return newCoor;
    }

    void setWidth(double width) {
        this->width = width;
    }

    double getWidth() const {
        return width;
    }

    void setHeight(double height) {
        this->height = height;
    }

    double getHeight() const {
        return height;
    }

    void setFixed(bool isFixed) {
        this->isFixed = isFixed;
    }

    bool getFixed() const {
        return isFixed;
    }

private:
    bool isFixed;
    Coor oriCoor;
    Coor newCoor;
    double height;
    double width;
    string name;
};



struct cmp{
    bool operator()(const Cell* a, const Cell* b)const{
        if(a->getNewCoor().x == b->getNewCoor().x)
            return a->getNewCoor().y < b->getNewCoor().y;
        else
            return a->getNewCoor().x < b->getNewCoor().x;
    }
};


class PlacementRow{
    public:
        Coor startCoor;
        double siteWidth;
        double siteHeight;
        int NumOfSites;
        multiset <Cell*, cmp> cellSet;


        PlacementRow(Coor startCoor, double siteWidth, double siteHeight, int NumOfSites)
            : startCoor(startCoor), siteWidth(siteWidth), siteHeight(siteHeight), NumOfSites(NumOfSites) {}
        void addCell(Cell* cell){
            // cellSort cellToSort(cell);
            cellSet.insert(cell);
        }
        void deleteCell(Cell* cell){
            // cellSort cellToSort(cell);
            cellSet.erase(cell);
        }
};


class Die{
private:
    double alpha;
    double beta;
    double leftX;
    double lowY;
    double rightX;
    double highY;
public:
// Cell
    unordered_map<string, Cell*> allCell;
//placement row
    vector<PlacementRow*> PlacementRows;
//OptCell
    vector<Cell*> OptCell;
    vector<vector<string>> OptRemoveCell;

    Die():alpha(0), beta(0), leftX(0), lowY(0), rightX(0), highY(0){};
    ~Die(){};
    void setDieAlpha(double alpha){
        this->alpha = alpha;
    }
    void setDieBeta(double beta){
        this->beta = beta;
    }
    void setDieSize(double x1, double x2, double y1, double y2){
        leftX = x1;
        rightX = x2;
        lowY = y1;
        highY = y2;
    }

    double getAlpha(){
        return alpha;
    }
    double getBeta(){
        return beta;
    }
    double getLeftX(){
        return leftX;
    }
    double getLowY(){
        return lowY;
    }
    double getRightX(){
        return rightX;
    }
    double getHighY(){
        return highY;
    }

    void addCell(Cell* cell){
        allCell[cell->getName()] = cell;
    }
    void eraseCell(string name){
        allCell.erase(name);
    }
    void addOptCell(Cell* cell){
        OptCell.push_back(cell);
    }
    void addOptRemoveCell(vector<string> & name){
        OptRemoveCell.push_back(name);
    }
    void addPlacementRow(PlacementRow *placementRow){
        PlacementRows.push_back(placementRow);
    } 

};

struct MoveCell{
    string name;
    Coor coor;
    MoveCell(string& n, Coor& c){
        name = n;
        coor = c;
    }
};

struct ChangeList{
    vector<Coor> placeCoorList;
    vector<vector<MoveCell>> MoveCellList;
};

class Evaluator{
    private:
        int moveTimes;
        unordered_map<string, Cell*> totalMoveCell;

    public:
        Evaluator():moveTimes(0){};
        void recordMoveCell(Cell* cell){
            if(totalMoveCell.count(cell->getName()) == 0)
                totalMoveCell[cell->getName()] = cell;
        }
        unordered_map<string, Cell*>& getTotalMoveCell(){
            return totalMoveCell;
        }
        void addMoveTimes(){
            moveTimes++;
        }
        int getMoveTimes(){
            return moveTimes;
        }
};



void parseLGfile(const std::string &filename);
void parseOPTfile(const std::string &filename);
void parseOUTPUTfile(const std::string &filename);
bool overlap(Cell* cell1, Cell* cell2);