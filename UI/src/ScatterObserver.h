#ifndef SCATTEROBSERVER_H
#define SCATTEROBSERVER_H

#include <list>


class IScatterObj{
public:
    virtual void OnScatterChanged(bool showRegion, bool scatter_ver2) = 0;
};

class ScatterObserver
{
public:
    ScatterObserver();
    ~ScatterObserver();

    void addObserver(IScatterObj *ob){
        observers.push_back(ob);
    }

    void notifyObservers(bool showRegion, bool scatter_ver2);

private:
   std::list<IScatterObj *> observers;
};


#endif // SCATTEROBSERVER_H
