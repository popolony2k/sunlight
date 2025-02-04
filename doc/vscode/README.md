# VSCode IDE configuration

If you want use VSCode to configure and launch project samples, there's some JSON samples ready for using inside VSCode IDE.

* [IDE configuration JSON files](#ide-configuration-json-files)
* [Using a specific settings.json for your operating system](#using-a-specific-settingsjson-for-your-operating-system)
* [VS Code Settings for Mac Windows and Linux extension](#vs-code-settings-for-mac-windows-and-linux-extension)
- [Launching samples executables](#launching-samples-executables)


## IDE configuration JSON files

Create a folder named .vscode to `sunlight` root path. 

```shell
cd sunlight
md .vscode
```

Copy whole folder content **sunlight/doc/vscode/vscode_sample** into the created **.vscode** folder.


```shell
cp doc/vscode/.vscode_sample/* ./vscode
```

The **launch.json** file has configurations for all supported `sunlight` operating systems (Mac, Windows and Linux).

## Using a specific settings.json for your operating system

The previous copy operation copies several settings with specific OS configuration. For example in our case it configures CMake to build all samples by default when using VSCode IDE.

There's 3 settings ready for using on your preferred system.

* settings.linux.json Specific settings for Linux;
* settings.macos.json Specific settings for MacOs;
* settings.windows.json Specific settings for Windows (with vckpg configuration)

For example, consider you're developing on Linux platform, so you could use this ready made **settings.linux.json** by copying it to the **settings.json** file of your **.vscode** folder present on root path of your `sunlight` project copy.

```shell
cd sunlight/.vscode
cp settings.linux.json settings.json
```

## VS Code Settings for Mac Windows and Linux extension

The other way is by installing [VS Code Settings for Mac Windows and Linux](https://marketplace.visualstudio.com/items?itemName=franmastromarino.vs-code-settings-os) VSCode extension.

If you prefer using this extension instead manual configuration described previsously, just follow the previous section [IDE configuration JSON files](#ide-configuration-json-files), install this extension and restart VSCode.
After this, each modification made on your specific JSON settings (eg. **settings.linux.json**), will be reflected automatically on main settings.json.  


## Launching samples executables

The **launch.json** was written to execute the compiled binary based on opened source code document.

For example, if in your VSCode the **tilemaprenderer_test.h** or **tilemaprenderer_test.cpp** is open on the active tab the VSCode launcher will try to start its related executable (**tilemaprenderer_test.exe** on Windows or **tilemaprenderer_test** on Linux or Mac), if this sample was already compiled previously, of course.
