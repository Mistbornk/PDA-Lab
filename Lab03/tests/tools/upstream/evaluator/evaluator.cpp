#include "evaluator.h"

void parseLGfile(const std::string &filename, Die& die){

    ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "Alpha") {
            double alpha;
            iss >> alpha;
            die.setDieAlpha(alpha);
        } else if (key == "Beta") {
            double beta;
            iss >> beta;
            die.setDieBeta(beta);
        } else if (key == "DieSize") {
            double x1, y1, x2, y2;
            iss >> x1 >> y1 >> x2 >> y2;
            die.setDieSize(x1, x2, y1,y2);
        } else if (key.find("FF_") == 0 || key.find("C") == 0) {
            double x, y, width, height;
            std::string fixedStr;
            iss >> x >> y >> width >> height >> fixedStr;
            bool isFixed = (fixedStr == "FIX");
            Cell* cell = new Cell;
            cell->setName(key);
            cell->setOriCoor(Coor(x, y));
            cell->setNewCoor(Coor(x, y));
            cell->setWidth(width);
            cell->setHeight(height);
            cell->setFixed(isFixed);
            die.addCell(cell);
        } else if (key == "PlacementRows") {
            double x, y, siteWidth, siteHeight;
            int numOfSites;
            iss >> x >> y >> siteWidth >> siteHeight >> numOfSites;
            PlacementRow * row = new PlacementRow(Coor(x, y), siteWidth, siteHeight, numOfSites);
            die.addPlacementRow(row);
        }
    }
    sort(die.PlacementRows.begin(), die.PlacementRows.end(), [](PlacementRow* a, PlacementRow* b){
        return a->startCoor.y < b->startCoor.y;
    });


    file.close();
}

void parseOPTfile(const std::string &filename, Die& die){
    
    ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "Banking_Cell:") {
            std::string dummy, newCellName;
            std::vector<std::string> cellsToRemove;
            double x, y, width, height;

            // Read cells to remove
            while (iss >> dummy && dummy != "-->") {
                cellsToRemove.push_back(dummy);
            }

            die.addOptRemoveCell(cellsToRemove);

            // Read new cell details
            iss >> newCellName >> x >> y >> width >> height;

            // Add new cell
            Cell* cell = new Cell;
            cell->setName(newCellName);
            cell->setOriCoor(Coor(x, y));
            cell->setNewCoor(Coor(x, y));
            cell->setWidth(width);
            cell->setHeight(height);
            cell->setFixed(false);
            die.addOptCell(cell);
        }
    }
    file.close();

}

void parseOUTPUTfile(const std::string &filename, Die& die, ChangeList &changeList){
    
    ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file: " << filename << std::endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        double x, y;
        iss >> x >> y;
        changeList.placeCoorList.push_back(Coor(x,y));

        getline(file, line);
        iss.clear();
        iss.str(line);
        int moveCellNum;
        iss >> moveCellNum;
        vector<MoveCell> moveVector;
        for(int i = 0;i < moveCellNum; i++){
            getline(file, line);
            iss.clear();
            iss.str(line);
            string name;
            iss >> name >> x >> y;
            Coor coor = Coor(x,y);
            MoveCell moveCell(name, coor);
            moveVector.push_back(moveCell);
        }
        changeList.MoveCellList.push_back(moveVector);
    }
    file.close();
}

bool checkFixed(vector<Cell*>& moveCells){
    for(size_t i = 0; i < moveCells.size()-1; i++){
        if(moveCells[i]->getFixed()){
            cerr << moveCells[i]->getName() << " is fixed, can't move !!" << endl;
            return false;
        }
    }
    return true;
}

bool checkBoundary(Die& die,vector<Cell*>& moveCells){
    for (const auto& cell : moveCells) {
        Coor coor = cell->getNewCoor();
        if (coor.x < die.getLeftX() || coor.x + cell->getWidth() > die.getRightX() ||
            coor.y < die.getLowY() || coor.y + cell->getHeight() > die.getHighY()) {
            std::cerr << "Error: Cell " << cell->getName() << " is out of die boundary" << std::endl;
            return false;
        }
    }
    return true;
}

bool checkOnsite(Die& die,vector<Cell*>& moveCells){
    for (const auto& cell : moveCells) {
        bool isOnSite = false;
        for (const auto& row : die.PlacementRows) {
            if (cell->getNewCoor().x == floor(cell->getNewCoor().x) && cell->getNewCoor().y == row->startCoor.y) {
                isOnSite = true;
                break;
            }
        }
        if (!isOnSite) {
            std::cerr << "Error: Cell " << cell->getName() << " is not on site of the placement row" << std::endl;
            return false;
        }
    }
    return true;
}
bool checkOverlap(Die& die,vector<Cell*>& moveCells){
    for (const auto& cell : moveCells) {
        bool isOverlap = false;
        // find placement row range
        double rowHeight = die.PlacementRows[0]->siteHeight;
        double rowInitalY = die.PlacementRows[0]->startCoor.y;
        int rowIdxLow = floor((cell->getNewCoor().y - rowInitalY)/rowHeight);
        int rowIdxHigh = ceil((cell->getNewCoor().y + cell->getHeight() - rowInitalY)/rowHeight);
        // check each placement row cell set
        for(int rowIdx = rowIdxLow; rowIdx < min(rowIdxHigh, (int)die.PlacementRows.size()); rowIdx++){
            multiset <Cell*, cmp> &cellSet = die.PlacementRows[rowIdx]->cellSet;
            
            // check if Cell exist
            auto it =  cellSet.find(cell);

            // for same coordinate cells
            if((*it) != cell){
                std::cerr << "Error: Cell " << cell->getName() << " and " << (*it)->getName() << " is overlap" << std::endl;
                std::cerr << "       Cell " << cell->getName() << " x:" << cell->getNewCoor().x << "~" << cell->getNewCoor().x + cell->getWidth() << " y:" << cell->getNewCoor().y << "~" << cell->getNewCoor().y + cell->getHeight() << std::endl;
                std::cerr << "       Cell " << (*it)->getName() << " x:" << (*it)->getNewCoor().x << "~" << (*it)->getNewCoor().x + (*it)->getWidth() << " y:" << (*it)->getNewCoor().y << "~" << (*it)->getNewCoor().y + (*it)->getHeight() << std::endl;
                return false;
            }

            if(it != cellSet.end()){
                auto preCellit = it;
                auto nxtCellit = it;
                // check if overlap from previous cell 
                if(it != cellSet.begin()){
                    preCellit--;
                    Cell* preCell = (*preCellit);
                    // cout << "preCell " << cell->getName() << " " << preCell->getName() << endl;
                    isOverlap = overlap(cell, preCell);
                    if (isOverlap) {
                        std::cerr << "Error: Cell " << cell->getName() << " and " << preCell->getName() << " is overlap" << std::endl;
                        std::cerr << "       Cell " << cell->getName() << " x:" << cell->getNewCoor().x << "~" << cell->getNewCoor().x + cell->getWidth() << " y:" << cell->getNewCoor().y << "~" << cell->getNewCoor().y + cell->getHeight() << std::endl;
                        std::cerr << "       Cell " << preCell->getName() << " x:" << preCell->getNewCoor().x << "~" << preCell->getNewCoor().x + preCell->getWidth() << " y:" << preCell->getNewCoor().y << "~" << preCell->getNewCoor().y + preCell->getHeight() << std::endl;
                        return false;
                    }
                }else{
                    // cout << "Not previous cell" << endl;
                }
                // check if overlap from next cell 
                if(++nxtCellit != cellSet.end()){
                    Cell* nxtCell = (*nxtCellit);
                    // cout << "nxtCell " << cell->getName() << " " << nxtCell->getName() << endl;
                    isOverlap = overlap(cell, nxtCell);
                    if (isOverlap) {
                        std::cerr << "Error: Cell " << cell->getName() << " and " << nxtCell->getName() << " is overlap" << std::endl;
                        std::cerr << "       Cell " << cell->getName() << " x:" << cell->getNewCoor().x << "~" << cell->getNewCoor().x + cell->getWidth() << " y:" << cell->getNewCoor().y << " ~ " << cell->getNewCoor().y + cell->getHeight() << std::endl;
                        std::cerr << "       Cell " << nxtCell->getName() << " x:" << nxtCell->getNewCoor().x << "~" << nxtCell->getNewCoor().x + nxtCell->getWidth() << " y:" << nxtCell->getNewCoor().y << " ~ " << nxtCell->getNewCoor().y + nxtCell->getHeight() << std::endl;
                        return false;
                    }
                }else{
                    // cout << "Not next cell" << endl;
                }
            }else{
                cerr << cell->getName() << " Not found" << endl;
                cerr << "Something Wrong. Please contact TA. Thanks!" << endl;
                return false;
            }
        }
        
    }
    return true;
}

bool overlap(Cell* cell1, Cell* cell2){
    Cell* cellUp = (cell1->getNewCoor().y > cell2->getNewCoor().y) ? cell1 : cell2;
    Cell* cellLow = (cell1->getNewCoor().y > cell2->getNewCoor().y) ? cell2 : cell1;
    if(cellUp->getNewCoor().y >= cellLow->getNewCoor().y + cellLow->getHeight()){
        return false;
    }
    Cell* cellLeft = (cell1->getNewCoor().x < cell2->getNewCoor().x) ? cell1 : cell2;
    Cell* cellRight = (cell1->getNewCoor().x < cell2->getNewCoor().x) ? cell2 : cell1;
    if(cellRight->getNewCoor().x >= cellLeft->getNewCoor().x + cellLeft->getWidth()){
        return false;
    }
    return true;
}

void initialPlacementRowSet(Die& die){
    double rowHeight = die.PlacementRows[0]->siteHeight;
    double rowInitalY = die.PlacementRows[0]->startCoor.y;
    for(const auto& cell_pair : die.allCell){
        Cell* cell = cell_pair.second;
        int rowIdxLow = floor((cell->getNewCoor().y - rowInitalY)/rowHeight);
        int rowIdxHigh = ceil((cell->getNewCoor().y + cell->getHeight() - rowInitalY)/rowHeight);
        for(int rowIdx = rowIdxLow; rowIdx < min(rowIdxHigh, (int)die.PlacementRows.size()); rowIdx++){
            die.PlacementRows[rowIdx]->addCell(cell);
        }
    }
    // debug
    // for(size_t rowIdx = 0; rowIdx <= (size_t)rowNum; rowIdx++){
        
    //     auto it = die.PlacementRows[rowIdx]->cellSet.begin();
    //     while(it != die.PlacementRows[rowIdx]->cellSet.end()){
    //         cout << (*it)->getName() << " ";
    //         it++;
    //     }
    //     cout << endl;
    // }
}

void placementRowEraseCell(Die& die, Cell* cell){
    double rowHeight = die.PlacementRows[0]->siteHeight;
    double rowInitalY = die.PlacementRows[0]->startCoor.y;
    int rowIdxLow = floor((cell->getNewCoor().y - rowInitalY)/rowHeight);
    int rowIdxHigh = ceil((cell->getNewCoor().y + cell->getHeight() - rowInitalY)/rowHeight);
    for(int rowIdx = rowIdxLow; rowIdx < min(rowIdxHigh, (int)die.PlacementRows.size()); rowIdx++){
        die.PlacementRows[rowIdx]->deleteCell(cell);
    }
}


void placementRowAddCell(Die& die, Cell* cell){
    double rowHeight = die.PlacementRows[0]->siteHeight;
    double rowInitalY = die.PlacementRows[0]->startCoor.y;
    int rowIdxLow = floor((cell->getNewCoor().y - rowInitalY)/rowHeight);
    int rowIdxHigh = ceil((cell->getNewCoor().y + cell->getHeight() - rowInitalY)/rowHeight);
    for(int rowIdx = rowIdxLow; rowIdx < min(rowIdxHigh, (int)die.PlacementRows.size()); rowIdx++){
        die.PlacementRows[rowIdx]->addCell(cell);
    }
}


bool sanityCheck(Die& die, ChangeList &changeList, Evaluator &evaluator, const int& round){
    die.addCell(die.OptCell[round]);

    // erase removeCell
    for(size_t erase_i = 0; erase_i < die.OptRemoveCell[round].size(); erase_i++){
        Cell* deleteCell = die.allCell[die.OptRemoveCell[round][erase_i]];
        // delete cell in allCell
        die.eraseCell(die.OptRemoveCell[round][erase_i]);
        
        // delete cell in all placement row
        placementRowEraseCell(die, deleteCell);

    }
    

    vector<Cell*> allMove;
    for(size_t move_i = 0; move_i < changeList.MoveCellList[round].size(); move_i++){
        string name = changeList.MoveCellList[round][move_i].name;
        Coor newCoor = changeList.MoveCellList[round][move_i].coor;

        //ensure move cell is exist
        bool find =  die.allCell.count(name);
        if(!find)
            cerr << name << " cell not exist !!" << endl;
        
        Cell* moveCell = die.allCell[name];
        // delete in origin placement row
        placementRowEraseCell(die, moveCell);

        moveCell->setNewCoor(newCoor);

        allMove.push_back(moveCell);
        evaluator.addMoveTimes();
        evaluator.recordMoveCell(moveCell);
    }
    
    for(const auto& moveCell : allMove){
        // add in placement row according to new coor
        placementRowAddCell(die, moveCell);
    }
            
    Cell* optCell = die.OptCell[round];
    optCell->setNewCoor(changeList.placeCoorList[round]);
    // add in placement row according to new coor
    placementRowAddCell(die, optCell);
    allMove.push_back(optCell);
    evaluator.recordMoveCell(optCell);


    
    /* debug : check update of placement row*/
    // for(size_t rowIdx = 0; rowIdx < die.PlacementRows.size(); rowIdx++){
    //     cout << "row" << rowIdx << ": ";
    //     for(const auto&s : die.PlacementRows[rowIdx]->cellSet){
    //         cout << s->getName() << " ";
    //     }
    //     cout << endl;
    // } 
    //     cout << endl;

    /* debug : checking move cell each round*/
    // for(size_t k = 0; k < allMove.size(); k++){
    //     cout << allMove[k]->getName() << " ";
    // }
    // cout << endl;

    /* sanity check */
    bool success = true;
    success = checkFixed(allMove);
    if(!success) return false;
    success = checkBoundary(die, allMove);
    if(!success) return false;
    success = checkOnsite(die, allMove);
    if(!success) return false;
    success = checkOverlap(die,allMove);
    if(!success) return false;



    /* debug : checking all cell each round*/
    // for(const auto& cell : die.allCell){
    //     cout << cell.first << endl;
    // }
    // cout << endl;
    return true;
}

double HPWL(const Coor& coor1, const Coor& coor2){
    return abs(coor1.x - coor2.x) + abs(coor1.y - coor2.y);
}

void evaluateScore(Die& die, Evaluator &evaluator){
    double score = 0;
    double alpha = die.getAlpha();
    double beta = die.getBeta();
    double timesCost = alpha * evaluator.getMoveTimes();
    score += timesCost;
    double totalDis = 0;
    // cout <<"displacement cells : "<< evaluator.getTotalMoveCell().size() << std::endl;
    // vector<Cell*> cellSort;
    // for(const auto& it : evaluator.getTotalMoveCell()){
    //     cellSort.push_back(it.second);
    // }
    // std::sort(cellSort.begin(), cellSort.end(), [](const Cell *a, const Cell *b) {
    //         return a->getName() < b->getName();
    //     });

    for(const auto& cell_pair : evaluator.getTotalMoveCell()){
        Cell* cell = cell_pair.second;
        totalDis += HPWL(cell->getNewCoor(), cell->getOriCoor());
        // cout << cell->getName() << " : " << HPWL(cell->getNewCoor(), cell->getOriCoor()) << endl;
        // cout << cell->getName() << " " << cell->getOriCoor().x << " " << cell->getOriCoor().y << " " << cell->getNewCoor().x << " " << cell->getNewCoor().y << endl;
    }
    double disCost = beta * totalDis;
    score += disCost;

    /* print pretty table */
    double disPercentage = disCost / score * 100;
    double timesPercentage = timesCost / score * 100;

    size_t numAfterDot = 3;
        std::vector<std::string> header = {"Cost", "-", "Weight", "Value", "Percentage(%)"};
        std::vector<std::vector<std::string>> rows = {
            {"Move Times", toStringWithPrecision(evaluator.getMoveTimes(), numAfterDot), toStringWithPrecision(die.getAlpha(), numAfterDot), toStringWithPrecision(timesCost, numAfterDot), toStringWithPrecision(timesPercentage, numAfterDot) + "(%)"},
            {"Total Distance", toStringWithPrecision(totalDis, numAfterDot), toStringWithPrecision(die.getBeta(), numAfterDot), toStringWithPrecision(disCost, numAfterDot), toStringWithPrecision(disPercentage, numAfterDot) + "(%)"},
            {"Total", "-", "-", toStringWithPrecision(score, numAfterDot), "100.00(%)"}
        };

        PrettyTable pt;
        pt.AddHeader(header);
		pt.AddRows(rows);
        pt.SetAlign(PrettyTable::Align::Internal);
		std::cout << pt << std::endl;

}

void printTable(){
    
}

int main(int argc, char *argv[]){
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <lg_filename> <opt_filename> <output_filename>" << std::endl;
        return 1;
    }

    Die die;
    ChangeList changeList;
    Evaluator evaluator;

    parseLGfile(argv[1], die);
    parseOPTfile(argv[2], die);
    int optTimes = die.OptCell.size();
    initialPlacementRowSet(die);
    parseOUTPUTfile(argv[3], die, changeList);
    
    /* debug : checking parse outputfile*/
    // cout << changeList.MoveCellList.size() << " " << changeList.placeCoorList.size() << endl;
    // for(size_t i = 0 ; i < changeList.MoveCellList.size(); i++){
    //     cout << changeList.placeCoorList[i].x << " " << changeList.placeCoorList[i].y << endl;
    //     for(size_t j = 0; j < changeList.MoveCellList[i].size(); j++){
    //         cout << changeList.MoveCellList[i][j].name  << endl;
    //     }
    // }

    if((int)changeList.placeCoorList.size() != optTimes){
        cerr << "Error : Number of optimize cells in output file not equal in opt_file !!" << std::endl;
        return 0;
    }
        
    
    bool sanityPass = true;
    for(int i = 0 ; i < optTimes; i++){
        sanityPass = sanityCheck(die, changeList, evaluator, i);
        if(!sanityPass) break;
    }
    /* Evaluator */
    if(sanityPass)
        evaluateScore(die, evaluator);



}
