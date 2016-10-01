#ifndef _BESTLOYALIST_H
#define _BESTLOYALIST_H

#include "standard.h"

class AllArmy : public DelayedTrick
{
    Q_OBJECT
public:
    Q_INVOKABLE AllArmy(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void takeEffect(ServerPlayer *target) const;
};

class MoreTroops : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE MoreTroops(Card::Suit suit, int number);
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class BeatAnother : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE BeatAnother(Card::Suit suit, int number);
    virtual bool isAvailable(const Player *player) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class BestLoyalistCardPackage : public Package
{
    Q_OBJECT

public:
    BestLoyalistCardPackage();
};

class BestLoyalistPackage : public Package
{
    Q_OBJECT

public:
    BestLoyalistPackage();
};

#endif