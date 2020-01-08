#include "rvmidioutportmodel.h"

#include "rvmidi.h"

RvMidiOutPortModel::RvMidiOutPortModel(RvMidi *rvmidi, QObject *parent)
    :QAbstractItemModel( parent), rvmidi( rvmidi)
{
    readPortList();
    connect( rvmidi, SIGNAL( writablePortConnectionChanged( RvMidiClientPortId, bool)), this, SIGNAL( modelReset()));
    connect( rvmidi, SIGNAL( portListChanged()), this, SLOT(readPortList()));
}

QVariant RvMidiOutPortModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
    {
        return portList.at( index.row()).name();
    }
    else if( role == Qt::EditRole || role == Qt::ToolTipRole)
    {
        QVariant val;
        val.setValue( portList.at( index.row()).ID());
        return val;
    }
    else if( role == Qt::CheckStateRole)
    {
        RvMidiClientPortId id = data( index, Qt::EditRole).value<RvMidiClientPortId>();
        bool val;
        val = rvmidi->isConnectedToWritablePort(id);
        return val==true? Qt::Checked : Qt::Unchecked;
    }

    return QVariant();
}

bool RvMidiOutPortModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role == Qt::CheckStateRole)
    {
        RvMidiClientPortId id = data( index, Qt::EditRole).value<RvMidiClientPortId>();
        bool boolval = value.toBool();
        if( boolval == true)
            return rvmidi->connectWritablePort( id);
        else
            return rvmidi->disconnectWritablePort( id);
    }

    return true;
}

Qt::ItemFlags RvMidiOutPortModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

int RvMidiOutPortModel::rowCount( const QModelIndex &) const
{
    return portList.size();
}

void RvMidiOutPortModel::readPortList()
{
    beginResetModel();
    portList = rvmidi->writableMidiPorts();
    endResetModel();
}
