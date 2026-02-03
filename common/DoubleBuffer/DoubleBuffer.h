#ifndef DOUBLEBUFFER_H
#define DOUBLEBUFFER_H
#include <list>

template<typename T>
class DoubleArray {

    struct NODE {
        T t;
        NODE* next;
    };

    int trigger;
    int size[2];
    NODE* header[2];
    NODE* tail[2];
public:
    DoubleArray() {
        trigger = 0;
        size[0] = size[1] = 0;
        header[0] = header[1] = NULL;
        tail[0] = tail[1] = NULL;
    }

    void write(T value);
    T read(int& error);
    std::list<T>& readall(std::list<T>& list);
};

template<typename T>
void DoubleArray<T>::write(T value)
{
    NODE *n = new NODE;
    int w_trigger = trigger;
    n->t = value;
    n->next = NULL;
    if (size[w_trigger] == 0) {
        header[w_trigger] = n;
        tail[w_trigger] = n;
        size[w_trigger]++;
    }
    else {
        tail[w_trigger]->next = n;
        tail[w_trigger] = n;
        size[w_trigger]++;
    }
}

template<typename T>
T DoubleArray<T>::read(int& error) {
    T reslut;
    int r_trigger = (trigger + 1)%2;
    if(header[r_trigger] == NULL && header[trigger] == NULL)
        error = -1;
    else
    {
        if(header[r_trigger] != NULL)
        {
            NODE* temp = header[r_trigger];
            header[r_trigger] = temp->next;
            reslut = temp->t;
            delete temp;
            size[r_trigger]--;

            if(header[r_trigger] == 0)//change write catch
            {
                tail[r_trigger] = NULL;
                size[r_trigger] = 0;
                trigger = r_trigger;
            }
        }
        else//read catch is null
        {
            trigger = r_trigger;//change write catch
            read();
        }
    }

    return reslut;
}

template<typename T>
std::list<T>& DoubleArray<T>::readall(std::list<T>& list) {
    int r_trigger = trigger;
    if (header[r_trigger] != 0) {
        // change b/a
        trigger = (r_trigger + 1)%2;
        // fetch a/b
        NODE* temp = header[r_trigger];
        while (temp) {
            list.push_back(temp->t);
            temp = temp->next;
        }
        // delete a/b
        temp = header[r_trigger];
        for (int i = 0; i < size[r_trigger]; ++i) {
            NODE* p = temp;
            temp = temp->next;
            delete p;
        }
        size[r_trigger] = 0;
        header[r_trigger] = NULL;
        tail[r_trigger] = NULL;
    }

    return list;
}


#endif // DOUBLEBUFFER_H
