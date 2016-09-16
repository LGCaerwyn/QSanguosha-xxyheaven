/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha-Hegemony.

    This game is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    See the LICENSE file for more details.

    Mogara
    *********************************************************************/

#ifndef _CHOOSE_OPTIONS_BOX_H
#define _CHOOSE_OPTIONS_BOX_H

#include "graphicsbox.h"

class Button;
class QGraphicsProxyWidget;

class ChooseOptionsBox : public GraphicsBox
{
    Q_OBJECT

public:
    explicit ChooseOptionsBox();

    QRectF boundingRect() const;

    inline void setSkillName(const QString &skillName)
    {
        this->skillName = skillName;
    }
    void clear();

public slots:
    void chooseOption(const QStringList &options);
    void reply();

private:
    QStringList options;
    QString skillName;
    QMap<QString, Button *> buttons;

    static const int defaultButtonHeight = 24;
    static const int minInterval = 15; //15
    static const int maxInterval = 40; //15
    static const int defaultBoundingWidth = 400;

    int getButtonWidth(const QString &card_name) const;
    int getInterval() const;

    QString translate(const QString &option) const;
};

#endif // _CHOOSE_OPTIONS_BOX_H
