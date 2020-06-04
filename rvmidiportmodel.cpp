#include "rvmidiportmodel.h"

#include "rvmidi.h"

RvMidiPortModel::RvMidiPortModel(RvMidi &rvmidi, Direction d, QObject *parent)
    :QAbstractItemModel( parent), direction(d), rvmidi( rvmidi)
{
    if( direction == ReadablePorts)
    {
        connect( &rvmidi, SIGNAL(readableMidiPortCreated( RvMidiClientPortId)), this, SLOT(portCreated(RvMidiClientPortId)));
        connect( &rvmidi, SIGNAL(readableMidiPortDestroyed( RvMidiClientPortId)), this, SLOT(portRemoved(RvMidiClientPortId)));
        connect( &rvmidi, SIGNAL(readableMidiPortConnected( RvMidiClientPortId)), this, SLOT(portConnected(RvMidiClientPortId)));
        connect( &rvmidi, SIGNAL(readableMidiPortDisconnected( RvMidiClientPortId)), this, SLOT(portDisconnected(RvMidiClientPortId)));
        portList = rvmidi.readableMidiPorts();
        connectedPortSet = rvmidi.connectedReadableMidiPortSet();
    }
    else
    {
        connect( &rvmidi, SIGNAL(writableMidiPortCreated( RvMidiClientPortId)), this, SLOT(portCreated(RvMidiClientPortId)));
        connect( &rvmidi, SIGNAL(writableMidiPortDestroyed( RvMidiClientPortId)), this, SLOT(portRemoved(RvMidiClientPortId)));
        connect( &rvmidi, SIGNAL(writableMidiPortConnected( RvMidiClientPortId)), this, SLOT(portConnected(RvMidiClientPortId)));
        connect( &rvmidi, SIGNAL(writableMidiPortDisconnected( RvMidiClientPortId)), this, SLOT(portDisconnected(RvMidiClientPortId)));
        portList = rvmidi.writableMidiPorts();
        connectedPortSet = rvmidi.connectedWritableMidiPortSet();
    }
}

QVariant RvMidiPortModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(role == Qt::DisplayRole)
    {
        return portList.at(row).name();
    }
    else if(role == Qt::CheckStateRole)
    {
        return connectedPortSet.contains(portList.at(row).ID())?Qt::Checked:Qt::Unchecked;
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
        int row = index.row();
        int intVal = value.toInt();
        if( intVal == Qt::Checked)
        {
            if( direction == ReadablePorts)
                rvmidi.connectToRedeablePort( portList.at(row).ID());
            else
                rvmidi.connectToWriteblePort( portList.at(row).ID());
        }
        else if( intVal == Qt::Unchecked )
        {
            if( direction == ReadablePorts)
                rvmidi.disconnectFromReadeablePort( portList.at(row).ID());
            else
                rvmidi.disconnectFromWriteblePort( portList.at(row).ID());
        }
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

void RvMidiPortModel::portConnected(RvMidiClientPortId id)
{
    connectedPortSet.insert(id);
}

void RvMidiPortModel::portDisconnected(RvMidiClientPortId id)
{
    connectedPortSet.remove(id);
}
