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

#include "chooseoptionsbox.h"
#include "engine.h"
#include "button.h"
#include "client.h"
#include "clientstruct.h"
#include "timed-progressbar.h"

#include <QGraphicsProxyWidget>

ChooseOptionsBox::ChooseOptionsBox()
{
}

QRectF ChooseOptionsBox::boundingRect() const
{
    int n = options.length();
    int allbuttonswidth = 0;
    foreach (const QString &card_name, options) {
        int buttonwidth = getButtonWidth(card_name);
        allbuttonswidth += buttonwidth;
    }
    int tempinterval = getInterval();
    return QRectF(0, 0, (allbuttonswidth + (n+1)*tempinterval), defaultButtonHeight);
}

void ChooseOptionsBox::chooseOption(const QStringList &options)
{
    //repaint background
    this->options = options;
    prepareGeometryChange();

    foreach (const QString &choice, options) {
        Button *button = new Button(translate(choice), QSizeF(getButtonWidth(choice), defaultButtonHeight));
        button->setObjectName(choice);
        buttons[choice] = button;
        button->setParentItem(this);

        QString original_tooltip = QString(":%1").arg(title);
        QString tooltip = Sanguosha->translate(original_tooltip);
        if (tooltip == original_tooltip) {
            original_tooltip = QString(":%1").arg(choice);
            tooltip = Sanguosha->translate(original_tooltip);
        }
        connect(button, &Button::clicked, this, &ChooseOptionsBox::reply);
        if (tooltip != original_tooltip)
            button->setToolTip(tooltip);
    }

    moveToCenter();
    moveBy(0, 125);
    show();
    int tempinterval = getInterval();
    int x = tempinterval;

    foreach (const QString &card_name, options) {
        QPointF apos;
        apos.setX(x);
        x += (tempinterval + getButtonWidth(card_name));
        apos.setY(0);
        buttons[card_name]->setPos(apos);
    }
}

void ChooseOptionsBox::reply()
{
    QString choice = sender()->objectName();
    if (choice.isEmpty())
        choice = options.first();
    ClientInstance->onPlayerMakeChoice(choice);
}

int ChooseOptionsBox::getButtonWidth(const QString &card_name) const
{
    QFontMetrics fontMetrics(Button::defaultFont());
    int width = fontMetrics.width(translate(card_name));
    // Otherwise it would look compact
    width += 30;
    return width;
}

int ChooseOptionsBox::getInterval() const
{
    int n = options.length();
    int allbuttonswidth = 0;
    foreach (const QString &card_name, options) {
        int buttonwidth = getButtonWidth(card_name);
        allbuttonswidth += buttonwidth;
    }
    int tempinterval = (defaultBoundingWidth-allbuttonswidth)/(n+1);
    if (tempinterval < minInterval)
        tempinterval = minInterval;
    if (tempinterval > maxInterval)
        tempinterval = maxInterval;
    return tempinterval;
}

QString ChooseOptionsBox::translate(const QString &option) const
{
    QString title = QString("%1:%2").arg(skillName).arg(option);
    QString translated = Sanguosha->translate(title);
    if (translated == title)
        translated = Sanguosha->translate(option);
    return translated;
}

void ChooseOptionsBox::clear()
{
    foreach(Button *button, buttons.values())
        button->deleteLater();

    buttons.values().clear();

    disappear();
}
