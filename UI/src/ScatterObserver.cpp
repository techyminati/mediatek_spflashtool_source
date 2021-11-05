#include "ScatterObserver.h"


ScatterObserver::ScatterObserver()
{
}

ScatterObserver::~ScatterObserver()
{

}

void ScatterObserver::notifyObservers(bool showRegion, bool scatter_ver2)
{
    for(std::list<IScatterObj*>::const_iterator it = observers.begin();
        it != observers.end(); ++it){
        (*it)->OnScatterChanged(showRegion, scatter_ver2);
    }
}

