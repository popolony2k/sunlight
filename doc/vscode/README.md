# VSCode

If you want use VSCode to configure and launch project samples, there's some JSON samples ready for using inside VSCode IDE.

## IDE configutation JSON files

Create a folder named .vscode to `sunlight` root path. 

```shell
cd sunlight
md .vscode
```

Copy whole folder content sunlight/doc/vscode/vscode_sample into the created .vscode folder.


```shell
cp doc/vscode/.vscode_sample/* ./vscode
```

The launch.json file has configurations for all supported `sunlight` operating systems (Mac, Windows and Linux).

## Launching samples

The launch.json was written to execute the compiled binary based on opened source code document.

For example, if in your VSCode the tilemaprenderer_test.h or tilemaprenderer_test.cpp is open on the active tab this launch will try to start its related executable (tilemaprenderer_test.exe on Windows or tilemaprenderer_test on Linux or Mac), if this sample was already compiled previously, of course.
