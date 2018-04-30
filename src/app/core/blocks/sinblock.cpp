#include "sinblock.h"

SinBlock::SinBlock(QGraphicsWidget* parent): Block(parent)
{
    BlockView* blockView = this->view();
    blockView->setSvgImage(":/res/image/sin_symbol.svg");
    this->setInputPorts({ new BlockPortValue(this->id(), true, blockView), });
    this->setOutputPort(new BlockPortValue(this->id(), false, blockView));

    blockView->initPortsViews();
}

MappedDataValues SinBlock::evaluate(const QList<MappedDataValues>& inputData)
{
    Q_ASSERT(this->inputMatchesPorts(inputData));

    return {{ "value", sin(inputData.at(0)["value"]) }};
}
