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
// Date: 10-4-2016
//
//
// Authors: M. R. Fortuin
//
//
// Purpose:
//
//
//////////////////////////////////////////////////////////////////////////

#ifndef OPENPSTD_MOUSECREATEDOMAINSTRATEGY_H
#define OPENPSTD_MOUSECREATEDOMAINSTRATEGY_H

#include <memory>
#include <QMouseEvent>
#include "GUI/operations/BaseOperation.h"
#include "GUI/Model.h"
#include "MouseStrategy.h"

namespace OpenPSTD
{
    namespace GUI
    {
        class MouseCreateDomainStrategy : public MouseStrategy
        {
        private:
            QVector2D _start;
            QVector2D _end;
        public:
            virtual void mousePressEvent(std::shared_ptr <OpenPSTD::GUI::Model> const &model, QMouseEvent *,
                                         QVector2D pos) override;

            virtual void mouseMoveEvent(std::shared_ptr <OpenPSTD::GUI::Model> const &model, QMouseEvent *mouseEvent,
                                        QVector2D pos) override;

            virtual void mouseReleaseEvent(std::shared_ptr <OpenPSTD::GUI::Model> const &model, QMouseEvent *mouseEvent,
                                           QVector2D pos) override;
        };
    }
}


#endif //OPENPSTD_MOUSECREATEDOMAINSTRATEGY_H
