#include <iostream>

#include <Recluse/RenderApi/RenderInstance.hpp>

using namespace std;

int main(int argc, char** argv)
{
    Recluse::RenderApi::ContextHandle context = Recluse::RenderApi::Context::create(Recluse::RenderApi::Api_Vulkan);
    cout << "Hello World!" << endl;

    Recluse::RenderApi::Context::free(context);
    return 0;
}