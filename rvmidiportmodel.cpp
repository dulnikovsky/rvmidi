#include "rvmidiportmodel.h"

#include "rvmidi.h"

RvMidiPortModel::RvMidiPortModel(RvMidi &rvmidi, Direction d, QObject *parent)
    :QAbstractItemModel( parent), direction(d), rvmidi( rvmidi)
{

}

QVariant RvMidiPortModel::data(const QModelIndex &index, int role) const
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
        if( direction == ReadablePorts)
            val = rvmidi.isConnectedToReadablePort(id);
        else
            val = rvmidi.isConnectedToWritablePort(id);

        return val==true? Qt::Checked : Qt::Unchecked;

    }

    return QVariant();
}

bool RvMidiPortModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role == Qt::CheckStateRole)
    {
        RvMidiClientPortId id = data( index, Qt::EditRole).value<RvMidiClientPortId>();
        bool boolval = value.toBool();
        if( direction == ReadablePorts)
        {
            if( boolval == true)
                return rvmidi.connectReadablePort( id);
            else
                return rvmidi.disconnectReadablePort( id);
        }
        else
        {
            if( boolval == true)
                return rvmidi.connectWritablePort( id);
            else
                return rvmidi.disconnectWritablePort( id);
        }
    }

    return true;
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
