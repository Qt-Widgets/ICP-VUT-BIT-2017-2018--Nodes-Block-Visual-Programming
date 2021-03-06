/**
 * Part of block editor project for ICP at FIT BUT 2017-2018.
 *
 * @package ICP-2017-2018
 * @authors Son Hai Nguyen xnguye16@stud.fit.vutbr.cz, Josef Kolář xkolar71@stud.fit.vutbr.cz
 * @date 06-05-2018
 * @version 1.0
 */

#include "blockcanvas.h"

#include <QPainter>
#include <QMimeData>
#include <QJsonDocument>
#include <QGraphicsScene>

BlockCanvas::BlockCanvas(QGraphicsWidget* parent) : ScrollArea(parent) {
    m_blockManager = new BlockManager;

    this->setGrooveColor(QColor(Qt::transparent));
    this->setHandleColor(QColor("#4c4c4c"));
    this->setAcceptDrops(true);
    this->setAcceptedMouseButtons(Qt::LeftButton);

    connect(m_blockManager, &BlockManager::blockDeleted, this, &BlockCanvas::blockDeleted);
    connect(m_blockManager, &BlockManager::joinDeleted, this, &BlockCanvas::joinDeleted);
}

BlockCanvas::~BlockCanvas() {
    delete m_blockManager;
}

void BlockCanvas::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setPen(QPen{QColor{"#8c8c8c"}, 3});
    painter->setOpacity(0.7);

    if (m_drawLine) {
        QPainterPath path;
        QPointF p1 = m_portStartPoint;
        QPointF p2 = m_portEndPoint;

        path.moveTo(p1);
        double startX = qMin(p1.x(), p2.x());
        const QPointF c1{startX + qAbs((p1.x() - p2.x()) / 2.), p1.y()};
        const QPointF c2{startX + qAbs((p1.x() - p2.x()) / 2.), p2.y()};

        path.cubicTo(c1, c2, p2);
        painter->drawPath(path);
    }

    if (m_dragOver) {
        painter->setPen(QColor(Qt::transparent));
        painter->setBrush(QColor("#efefef"));
        painter->drawRect(this->boundingRect());
    }

    painter->setFont(QFont("Montserrat", 20));
    painter->drawText(this->boundingRect(), Qt::AlignBottom | Qt::AlignRight, "Son Hai Nguyen, Josef Kolář ©2018");
    painter->restore();
}

void BlockCanvas::dragEnterEvent(QGraphicsSceneDragDropEvent* e) {
    if (e->mimeData()->hasFormat(BlockManager::blockMimeType())) {
        m_dragOver = true;
        e->setAccepted(true);
        this->update();
    } else
        e->setAccepted(false);
}

void BlockCanvas::dragLeaveEvent(QGraphicsSceneDragDropEvent* e) {
    Q_UNUSED(e);
    m_dragOver = false;
    this->update();
}

void BlockCanvas::dropEvent(QGraphicsSceneDragDropEvent* e) {
    if (m_disableDrop)
        return;

    m_dragOver = false;
    if (e->mimeData()->hasFormat(BlockManager::blockMimeType())) {
        QByteArray rawData{e->mimeData()->data(BlockManager::blockMimeType())};
        QDataStream data{&rawData, QIODevice::ReadOnly};
        QPoint hotspot;
        QByteArray jsonData;
        data >> hotspot;
        data >> jsonData;

        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        auto block = BlockManager::blockFromJson(doc.object(), this->container());
        m_blockManager->addBlock(block);

        block->view()->setCopyable(false);
        block->view()->setFlag(QGraphicsItem::ItemIsSelectable);
        block->view()->setFlag(QGraphicsItem::ItemIsMovable);

        block->view()->setPos(e->pos() - QPointF(hotspot));
        block->view()->setOutputVisible(false, false);
        block->view()->setInputsVisible(false, false);
        block->view()->setOutputVisible(true);
        block->view()->setInputsVisible(true);

        emit this->blockAdded(block->id());
    }

    this->update();
}

void BlockCanvas::mousePressEvent(QGraphicsSceneMouseEvent* e) {
    BlockPortView* portView = this->portViewAtPos(e->pos());

    if (portView == nullptr) {
        QGraphicsWidget::mousePressEvent(e);
        return;
    }

    this->dishighlightPorts(portView->portData()->type());
    m_portOrigStartPoint = e->pos();
    m_portStartPoint = portView->mapToItem(
            this, QPointF(0, portView->size().height() / 2.));
    m_drawLine = true;
}

void BlockCanvas::mouseMoveEvent(QGraphicsSceneMouseEvent* e) {
    m_portEndPoint = e->pos();
    this->update();
    QGraphicsWidget::mouseMoveEvent(e);
}

void BlockCanvas::mouseReleaseEvent(QGraphicsSceneMouseEvent* e) {
    BlockPortView* toPortView = this->portViewAtPos(e->pos());

    this->restoreHighlightPorts();
    if (!m_drawLine || toPortView == nullptr) {
        m_drawLine = false;
        m_portStartPoint = QPointF(-1, -1);;
        this->update();
        QGraphicsWidget::mouseReleaseEvent(e);
        return;
    }

    m_drawLine = false;
    m_portStartPoint = QPointF(-1, -1);;

    // check if it is relesed over input port
    if (toPortView->portData()->isOutput()) {
        this->update();
        return;
    }

    BlockPortView* outPortView = this->portViewAtPos(m_portOrigStartPoint);
    Block* fromBlock = m_blockManager->block(outPortView->portData()->blockId());
    Block* toBlock = m_blockManager->block(toPortView->portData()->blockId());
    PortIdentifier toPortId = toBlock->inputPorts().indexOf(toPortView->portData());

    if (outPortView->portData()->type() != toPortView->portData()->type()) {
        this->update();
        emit this->error(tr("Ports types are not the same."));
        return;
    }

    auto join = new Join{fromBlock->id(), 0, toBlock->id(), toPortId, this->container()};
    join->setBlockManager(m_blockManager);
    m_blockManager->addJoin(join);

    emit this->joinAdded(join->id());
    this->update();
}

BlockPortView* BlockCanvas::portViewAtPos(QPointF pos) const {
    QList<BlockPortView*> portViews;
    for (auto singlePortView: this->scene()->items(pos + this->pos())) {
        BlockPortView* castedPortView = dynamic_cast<BlockPortView*>(singlePortView);
        if (castedPortView != nullptr)
            portViews.append(castedPortView);
    }

    if (!portViews.length())
        return nullptr;
    return portViews.at(0);
}

bool BlockCanvas::schemeValidity() const {
    bool valid = true;
    for (auto block: m_blockManager->blocks().values())
        valid = valid && block->validInputs();

    return valid;
}

bool BlockCanvas::cycled() const {
    QSet<Identifier> expansion, toBeExpanded, endBlocks, passed, notPassed;
    notPassed = m_blockManager->blocks().keys().toSet();

    for (auto block: m_blockManager->blocks().values()) {
        if (!block->outputPort()->isConnected())
            endBlocks.insert(block->id());
    }

    while (!notPassed.isEmpty()) {
        Identifier blockId = notPassed.toList().at(0);
        notPassed.remove(blockId);
        bool expasionEnded = false;

        expansion.clear();
        expansion.insert(blockId);
        passed.clear();

        do {
            notPassed -= expansion;
            if (!(expansion & passed).isEmpty())
                return true;
            if ((expansion - endBlocks).isEmpty()) {
                expasionEnded = true;
                break;
            }
            passed |= expansion;

            // expand
            for (Identifier id: expansion)
                toBeExpanded |= m_blockManager->blockBlocksOutputs(id);
            expansion = toBeExpanded;
            toBeExpanded.clear();

        } while (!expasionEnded);
    }

    return false;
}

BlockManager* BlockCanvas::manager() const {
    return m_blockManager;
}

QList<Identifier> BlockCanvas::blockComputeOrder() {
    QList<Identifier> computedBlocks;
    while (computedBlocks.size() < m_blockManager->blocks().values().length()) {
        for (auto block: m_blockManager->blocks().values()) {
            if (computedBlocks.contains(block->id()))
                continue;
            const QSet<Identifier> blockOuts = m_blockManager->blockBlocksInputs(block->id());
            bool blockEvaluable = blockOuts.isEmpty() ||
                                  (blockOuts - computedBlocks.toSet()).isEmpty();
            if (blockEvaluable)
                computedBlocks.append(block->id());
        }
    }

    return computedBlocks;
}

void BlockCanvas::evaluateBlock(Identifier blockId) {
    Block* block = m_blockManager->block(blockId);
    MappedDataValues res = block->evaluate(block->view()->values());
    QList<QPair<Identifier, Identifier> > blocksTopropagate =
            m_blockManager->blockOutputs(block->id());

    block->outputPort()->setValue(res);
    for (auto outData: blocksTopropagate) {
        m_blockManager->blocks()[outData.first]->inputPorts()
                .at(static_cast<int>(outData.second))->setValue(res);
    }
}

void BlockCanvas::restoreHighlightPorts() {
    for (auto block: m_blockManager->blocks()) {
        for (auto port: block->inputPorts()) {
            if (!port->isOutput() && !port->isConnected())
                port->view()->animateShow();
        }
    }
}

void BlockCanvas::dishighlightPorts(Type::TypeE type) {
    for (auto block: m_blockManager->blocks()) {
        for (auto port: block->inputPorts()) {
            if (!port->isOutput() && !port->isConnected() && port->type() != type)
                port->view()->animatePartialHide(0.3);
        }
    }
}

void BlockCanvas::evaluate() {
    // check if ports are valid
    if (this->cycled()) {
        emit this->error(tr("Scheme has cycle."));
        return;
    }

    if (!this->schemeValidity()) {
        emit this->error(tr("Scheme has invalid inputs."));
        return;
    }

    // compute available blocks
    for (Identifier blockId: this->blockComputeOrder())
        this->evaluateBlock(blockId);
}

void BlockCanvas::debug() {
    // check if ports are valid
    if (!this->schemeValidity()) {
        emit this->error(tr("Scheme has invalid inputs."));
        return;
    }

    if (this->cycled()) {
        emit this->error(tr("Scheme has cycle."));
        return;
    }

    QList<Identifier> computeOrder = this->blockComputeOrder();
    if (m_debugIteration >= computeOrder.length()) {
        this->stopDebug();
        return;
    }

    if (m_debugIteration == 0)
        this->scene()->clearSelection();
    m_blockManager->setDisableDelete(true);
    this->setDisableDrop(true);

    Block* block = m_blockManager->block(computeOrder.at(m_debugIteration));
    block->view()->setSelected(true);

    if (m_debugIteration > 0)
        m_blockManager->block(computeOrder.at(m_debugIteration - 1))->view()->setSelected(false);
    this->evaluateBlock(block->id());

    m_debugIteration++;
    emit this->debugStateChanged(true);
}

void BlockCanvas::stopDebug() {
    m_debugIteration = 0;
    m_blockManager->setDisableDelete(false);
    this->scene()->clearSelection();
    this->setDisableDrop(false);
    emit this->debugStateChanged(false);
}

void BlockCanvas::setDisableDrop(bool v) {
    m_disableDrop = v;
}

void BlockCanvas::clear() {
    QList<Identifier> blocks = m_blockManager->blocks().keys();
    for (auto blockId: blocks)
        m_blockManager->deleteBlock(blockId);
}
