#ifndef MOL_PACKAGE_H
#define MOL_PACKAGE_H

#include "package.h"
#include "card.h"
#include "standard.h"

class MOLPackage : public Package
{
    Q_OBJECT

public:
    MOLPackage();
};

class QingyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QingyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhanyiViewAsBasicCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhanyiViewAsBasicCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ZhanyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhanyiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif


