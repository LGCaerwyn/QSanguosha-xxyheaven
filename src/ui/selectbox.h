#ifndef SELECTBOX
#define SELECTBOX

#include "graphicsbox.h"
#include "qsanbutton.h"

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

    int getButtonWidth(const QString &card) const;

    QString translate(const QString &option) const;

    QStringList options;
    QString skill_name;

    QMap<QString, QStringList> card_list;

    QMap<QString, QSanButton *> buttons;

    static const int defaultButtonHeight;
    static const int interval;
    static const int defaultBoundingWidth;
};

#endif // GUHUOBOX

