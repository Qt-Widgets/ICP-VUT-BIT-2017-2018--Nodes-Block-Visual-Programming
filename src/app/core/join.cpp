//
// Created by thejoeejoee on 20.3.18.
//

#include "join.h"

Join::Join(Identifier fromBlock, PortIdentifier fromPort, Identifier toBlock, PortIdentifier toPort, QGraphicsItem* parent): m_fromBlock{fromBlock}, m_fromPort{fromPort}, m_toBlock{toBlock}, m_toPort{toPort}
{
    m_view = new JoinView{parent};
}

Identifier Join::fromBlock() const
{
    return m_fromBlock;
}

PortIdentifier Join::fromPort() const
{
    return m_fromPort;
}

Identifier Join::toBlock() const
{
    return m_toBlock;
}

PortIdentifier Join::toPort() const
{
    return m_toPort;
}

JoinView* Join::view() const
{
    return m_view;
}
