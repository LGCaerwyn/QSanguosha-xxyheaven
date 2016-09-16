#include "selectbox.h"
#include "button.h"
#include "engine.h"
#include "standard.h"
#include "clientplayer.h"
#include "skin-bank.h"
#include "roomscene.h"

#include <QPropertyAnimation>
#include <QGraphicsSceneMouseEvent>

const int SelectBox::defaultButtonHeight = 24;
const int SelectBox::minInterval = 15; //15
const int SelectBox::maxInterval = 40; //15
const int SelectBox::defaultBoundingWidth = 400;

SelectBox::SelectBox(const QString &skillname, const QStringList &options)
{
    this->skill_name = skillname;
    this->options = options;
}

QRectF SelectBox::boundingRect() const
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

bool SelectBox::isButtonEnable(const QString &card_name) const
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill == NULL)
        return false;
    return skill->buttonEnabled(card_name);
}

int SelectBox::getInterval() const
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

int SelectBox::getButtonWidth(const QString &card_name) const
{
    QFontMetrics fontMetrics(Button::defaultFont());
    int width = fontMetrics.width(translate(card_name));
    // Otherwise it would look compact
    width += 30;
    return width;
}

void SelectBox::popup()
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill == NULL || !skill->buttonEnabled()) {
        emit onButtonClick();
        return;
    }
    RoomSceneInstance->getDasboard()->unselectAll();
    RoomSceneInstance->getDasboard()->retractAllSkillPileCards();
    RoomSceneInstance->enableTargets(NULL);
    RoomSceneInstance->getDasboard()->disableAllCards();
    RoomSceneInstance->setCancelButton(true);
    RoomSceneInstance->current_select_box = this;

    foreach (const QString &card_name, options) {
        Button *button = new Button(translate(card_name), QSizeF(getButtonWidth(card_name), defaultButtonHeight));
        button->setObjectName(card_name);
        button->setParentItem(this);

        buttons[card_name] = button;

        button->setEnabled(isButtonEnable(card_name));

        QString original_tooltip = QString(":%1").arg(title);
        QString tooltip = Sanguosha->translate(original_tooltip);
        if (tooltip == original_tooltip) {
            original_tooltip = QString(":%1").arg(card_name);
            tooltip = Sanguosha->translate(original_tooltip);
        }
        connect(button, &Button::clicked, this, &SelectBox::reply);
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

void SelectBox::reply()
{
    Self->tag.remove(skill_name);
    QString choice = sender()->objectName();
    Self->tag[skill_name] = choice;
    emit onButtonClick();
    clear();
}

void SelectBox::clear()
{
    RoomSceneInstance->current_select_box = NULL;

    if (sender() != NULL && Self->tag[skill_name] == sender()->objectName() && Sanguosha->getViewAsSkill(skill_name) != NULL)
        RoomSceneInstance->getDasboard()->updatePending();
    else if (Self->getPhase() == Player::Play)
        RoomSceneInstance->getDasboard()->enableCards();

    if (!isVisible())
        return;

    foreach(Button *button, buttons.values())
        button->deleteLater();

    buttons.values().clear();

    disappear();
}

QString SelectBox::translate(const QString &option) const
{
    QString title = QString("%1:%2").arg(skill_name).arg(option);
    QString translated = Sanguosha->translate(title);
    if (translated == title)
        translated = Sanguosha->translate(option);
    return translated;
}
