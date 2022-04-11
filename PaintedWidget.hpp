// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#pragma once

#include <QWidget>

class PaintedWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PaintedWidget(QWidget *parent = nullptr);

public slots:
    void setImageData(const QByteArray &data);
    void setItemRect(const QRect &rect);
    void setClickPoint(const QPointF &point);
    QPointF scaledClickPoint();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QByteArray m_imageData;
    QRect m_itemRect;
    QPointF m_scaledPoint;
    QPointF m_clickPoint;

    float m_ratio = 1.0f;
};

