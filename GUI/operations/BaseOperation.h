//////////////////////////////////////////////////////////////////////////
// This file is part of openPSTD.                                       //
//                                                                      //
// openPSTD is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU General Public License as published by //
// the Free Software Foundation, either version 3 of the License, or    //
// (at your option) any later version.                                  //
//                                                                      //
// openPSTD is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of       //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        //
// GNU General Public License for more details.                         //
//                                                                      //
// You should have received a copy of the GNU General Public License    //
// along with openPSTD.  If not, see <http://www.gnu.org/licenses/>.    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Date:
//
//
// Authors:
//
//
// Purpose:
//
//
//////////////////////////////////////////////////////////////////////////

//
// Created by michiel on 18-7-2015.
//

#ifndef OPENPSTD_BASEOPERATION_H
#define OPENPSTD_BASEOPERATION_H

class OperationRunner;
class Reciever;
class BaseOperation;

#include "../Model.h"
#include <memory>

class OperationRunner
{
public:
    virtual void RunOperation(std::shared_ptr<BaseOperation> operation) = 0;
};

class Reciever
{
public:
    std::shared_ptr<Model> model;
    std::shared_ptr<OperationRunner> operationRunner;
};

class BaseOperation
{
public:
    virtual void Run(const Reciever &reciever) = 0;
};

class LambdaOperation: public BaseOperation
{
private:
    std::function<void (const Reciever &)> _func;

public:
    LambdaOperation(std::function<void (const Reciever &)> func);
    virtual void Run(const Reciever &reciever) override;
};


#endif //OPENPSTD_BASEOPERATION_H