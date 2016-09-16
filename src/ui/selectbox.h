#ifndef SELECTBOX
#define SELECTBOX

#include "graphicsbox.h"
#include "title.h"
#include "button.h"

class SelectBox : public GraphicsBox
{
    Q_OBJECT

public:
    SelectBox(const QString& skill_name, const QStringList &options);
    QString getSkillName()const
    {
        return skill_name;
    }

signals:
    void onButtonClick();

public slots:
    void popup();
    void reply();
    void clear();

protected:
    virtual QRectF boundingRect() const;

    bool isButtonEnable(const QString &card) const;

    int getInterval() const;
    int getButtonWidth(const QString &card) const;

    QString translate(const QString &option) const;

    QStringList options;
    QString skill_name;

    QMap<QString, QStringList> card_list;

    QMap<QString, Button *> buttons;

    static const int defaultButtonHeight;
    static const int minInterval;
    static const int maxInterval;
    static const int defaultBoundingWidth;
};

#endif // GUHUOBOX

