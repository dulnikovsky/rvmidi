#include "rvmidiinportmodel.h"

#include "rvmidi.h"

RvMidiInPortModel::RvMidiInPortModel(RvMidi *rvmidi, QObject *parent)
    :QAbstractItemModel( parent), rvmidi( rvmidi)
{
    readPortList();

    connect( rvmidi, SIGNAL( readablePortConnectionChanged( RvMidiClientPortId, bool)), this, SIGNAL( modelReset()));
    connect( rvmidi, SIGNAL( portListChanged()), this, SLOT(readPortList()));
}

QVariant RvMidiInPortModel::data(const QModelIndex &index, int role) const
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
        val = rvmidi->isConnectedToReadablePort(id);
        return val==true? Qt::Checked : Qt::Unchecked;
    }

    return QVariant();
}

bool RvMidiInPortModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role == Qt::CheckStateRole)
    {
        RvMidiClientPortId id = data( index, Qt::EditRole).value<RvMidiClientPortId>();
        bool boolval = value.toBool();
        if( boolval == true)
            return rvmidi->connectReadablePort( id);
        else
            return rvmidi->disconnectReadablePort( id);
    }

    return true;
}

Qt::ItemFlags RvMidiInPortModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

int RvMidiInPortModel::rowCount( const QModelIndex &) const
{
    return portList.size();
}

void RvMidiInPortModel::readPortList()
{
    beginResetModel();
    portList = rvmidi->readableMidiPorts();
    endResetModel();
}
