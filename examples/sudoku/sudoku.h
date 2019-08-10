//
// Created by john on 7/4/19.
//

#ifndef SUDOKU_SUDOKU_H
#define SUDOKU_SUDOKU_H

#include "base/Types.h"
#include "base/StringPiece.h"

muduo::string solveSudoku(const muduo::StringPiece& puzzle);
const int kCells = 81;
extern const char kNoSolution[];


#endif //SUDOKU_SUDOKU_H
