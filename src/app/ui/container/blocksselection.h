/**
 * Part of block editor project for ICP at FIT BUT 2017-2018.
 *
 * @package ICP-2017-2018
 * @authors Son Hai Nguyen xnguye16@stud.fit.vutbr.cz, Josef Kolář xkolar71@stud.fit.vutbr.cz
 * @date 06-05-2018
 * @version 1.0
 */

#ifndef BLOCKSSELECTION_H
#define BLOCKSSELECTION_H

#include <QGraphicsWidget>
#include <QGraphicsLinearLayout>
#include "scrollarea.h"


/**
 * Representation of aside block selection.
 */
class BlocksSelection : public ScrollArea {
    Q_OBJECT
    private:
        QGraphicsLinearLayout* m_layout;

    public:
        explicit BlocksSelection(QGraphicsWidget* parent = nullptr);

        /**
         * Paints selection list via painter.
         * @param painter painter
         * @param option style
         * @param widget optional qt parent
         */
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    public slots:
        /**
         * Adds new item to list.
         * @param item new item
         */
        void addItem(QGraphicsWidget* item);
};

#endif // BLOCKSSELECTION_H
