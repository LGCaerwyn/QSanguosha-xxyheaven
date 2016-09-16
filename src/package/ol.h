#ifndef OL_PACKAGE_H
#define OL_PACKAGE_H

#include "package.h"
#include "card.h"

class OLPackage : public Package
{
    Q_OBJECT

public:
    OLPackage();
};

class BifaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BifaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SongciCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SongciCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class AocaiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE AocaiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class DuwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DuwuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ZhoufuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhoufuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ShefuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShefuCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LuanzhanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LuanzhanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class QingshouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QingshouCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TianzuoDiscardCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TianzuoDiscardCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class GusheCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GusheCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif
