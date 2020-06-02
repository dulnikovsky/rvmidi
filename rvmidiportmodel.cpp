#include "rvmidiportmodel.h"

#include "rvmidi.h"

RvMidiPortModel::RvMidiPortModel(RvMidi &rvmidi, Direction d, QObject *parent)
    :QAbstractItemModel( parent), direction(d), rvmidi( rvmidi)
{
    if( direction == ReadablePorts)
    {
        connect( &rvmidi, SIGNAL(readableMidiPortCreated( RvMidiClientPortId)), this, SLOT(portCreated(RvMidiClientPortId)));
        connect( &rvmidi, SIGNAL(readableMidiPortDestroyed( RvMidiClientPortId)), this, SLOT(portRemoved(RvMidiClientPortId)));
    }
    else
    {
        connect( &rvmidi, SIGNAL(writableMidiPortCreated( RvMidiClientPortId)), this, SLOT(portCreated(RvMidiClientPortId)));
        connect( &rvmidi, SIGNAL(writableMidiPortDestroyed( RvMidiClientPortId)), this, SLOT(portRemoved(RvMidiClientPortId)));
    }
}

QVariant RvMidiPortModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
    {
        return portList.at(index.row()).name();
    }
    else if(role == Qt::CheckStateRole)
    {
        return Qt::Unchecked;
    }
    else if(role == Qt::ToolTipRole)
    {
        QVariant val;
        val.setValue(portList.at(index.row()).ID());
        return val;
    }

    return QVariant();
}

bool RvMidiPortModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role == Qt::CheckStateRole)
    {
        ;
        return true;
    }

    return false;
}

Qt::ItemFlags RvMidiPortModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

int RvMidiPortModel::rowCount( const QModelIndex &) const
{
    if( portList.empty())
    {
        if( direction == ReadablePorts)
            portList = rvmidi.readableMidiPorts();
        else
            portList = rvmidi.writableMidiPorts();
    }
    return portList.size();
}

void RvMidiPortModel::portCreated(RvMidiClientPortId id)
{
    beginResetModel();
    if( direction == ReadablePorts)
        portList = rvmidi.readableMidiPorts();
    else
        portList = rvmidi.writableMidiPorts();
    endResetModel();
}

void RvMidiPortModel::portRemoved(RvMidiClientPortId id)
{
    for(int i=0; i < portList.size(); i++)
    {
        if( portList[i].ID() == id)
        {
            beginResetModel();
            portList.removeAt(i);
            endResetModel();
            break;
        }
    }
}
