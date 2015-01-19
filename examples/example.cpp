#include <vector>
#include <iostream>

#include "../src/Socket.h"

#include "example.pb.h"

struct Object
{
public:
    int id;
    std::string vertices;
    std::string normals;
    std::string indices;
};

std::vector<Object*> objects;

void handleMessage(Arcus::Socket& socket, Arcus::MessagePtr message);

int main(int argc, char** argv)
{
    Arcus::Socket socket;

    socket.registerMessageType(2, &Example::ObjectList::default_instance());
    socket.registerMessageType(5, &Example::ProgressUpdate::default_instance());
    socket.registerMessageType(6, &Example::SlicedObjectList::default_instance());

    std::cout << "Connecting to server\n";
    socket.connect("127.0.0.1", 56789);

    while(true)
    {
        if(socket.state() == Arcus::SocketState::Connected)
        {
            auto message = socket.takeNextMessage();
            if(message)
            {
                handleMessage(socket, message);
            }
        }
        else if(socket.state() == Arcus::SocketState::Closed)
        {
            break;
        }
    }

    return 0;
}

void handleMessage(Arcus::Socket& socket, Arcus::MessagePtr message)
{
    auto objectList = dynamic_cast<Example::ObjectList*>(message);
    if(objectList)
    {
        for(auto object : objects)
        {
            delete object;
        }
        objects.clear();

        for(auto objectDesc : objectList->objects())
        {
            Object* obj = new Object();
            obj->id = objectDesc.id();
            obj->vertices = objectDesc.vertices();
            obj->normals = objectDesc.normals();
            obj->indices = objectDesc.indices();
            objects.push_back(obj);
        }

        auto msg = new Example::SlicedObjectList{};
        int progress = 0;
        for(auto object : objects)
        {
            auto slicedObject = msg->add_objects();
            slicedObject->set_id(object->id);

            for(int i = 0; i < 100; ++i)
            {
                auto polygon = slicedObject->add_polygons();
                polygon->set_points("abcdefghijklmnopqrstuvwxyz");
            }

            auto update = new Example::ProgressUpdate{};
            update->set_objectid(object->id);
            update->set_amount((float(++progress) / float(objects.size())) * 100.f);
            socket.sendMessage(update);
        }
        socket.sendMessage(msg);

        return;
    }
}
