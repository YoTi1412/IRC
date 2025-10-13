#include "Room.hpp"

Room::Room() : w1(0), w2(0), ties(0), played(0), setW1(0), setW2(0)
{

}

Room::Room(const std::string &nm) : name(nm), w1(0), w2(0), ties(0), played(0), setW1(0), setW2(0)
{

}

const std::string &Room::getName() const
{
    return name;
}

bool Room::isEmpty() const
{
    return p1.empty() && p2.empty();
}

bool Room::hasPlayer(const std::string &nick) const
{
    return nick == p1 || nick == p2;
}

bool Room::addPlayer(const std::string &nick)
{
    if (p1.empty())
    {
        p1 = nick; return true;
    }
    if (p2.empty() && nick != p1)
    {
        p2 = nick; return true;
    }
    return false;
}

void Room::removePlayer(const std::string &nick)
{
    if (nick == p1)
    {
        p1.clear(); c1.clear();
    }
    if (nick == p2)
    {
        p2.clear(); c2.clear();
    }
}

bool Room::isReady() const
{
    return !p1.empty() && !p2.empty();
}

bool Room::setChoice(const std::string &nick, const std::string &choice)
{
    if (nick == p1)
    {
        c1 = choice;
        return true;
    }
    if (nick == p2)
    {
        c2 = choice;
        return true;
    }
    return false;
}

bool Room::bothChose() const
{
    return !c1.empty() && !c2.empty();
}

void Room::clearChoices()
{
    c1.clear();
    c2.clear();
}

int Room::getPlayed() const
{
    return played;
}
int Room::getW1() const
{
    return w1;
}
int Room::getW2() const
{
    return w2;
}
int Room::getTies() const
{
    return ties;
}
const std::string &Room::getP1() const
{
    return p1;
}
const std::string &Room::getP2() const
{
    return p2;
}
const std::string &Room::getC1() const
{
    return c1;
}
const std::string &Room::getC2() const
{
    return c2;
}

void Room::applyOutcome(int outcome)
{
    if (outcome > 0)
        ++w1;
    else if (outcome < 0)
        ++w2;
    else
        ++ties;
    ++played;
}

bool Room::isOver() const
{
    return played >= 5;
}

int Room::getSetW1() const
{
    return setW1;
}

int Room::getSetW2() const
{
    return setW2;
}

void Room::addSetW1()
{
    ++setW1;
}

void Room::addSetW2()
{
    ++setW2;
}

void Room::resetRoundCounters()
{
    w1 = 0;
    w2 = 0;
    ties = 0;
    played = 0;
    c1.clear();
    c2.clear();
}
