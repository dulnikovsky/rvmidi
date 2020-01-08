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
        return portList.at(index.row()).name();
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

    return true;
}

Qt::ItemFlags RvMidiPortModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
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
