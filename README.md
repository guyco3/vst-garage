Do this bruh

```sh
(base) guycohen@Guys-MacBook-Air vst % xcodebuild -scheme "NewProject - Standalone Plugin"
(base) guycohen@Guys-MacBook-Air vst % open /Users/guycohen/dev/vst-garage/plugins/vst/NewProject/Builds/MacOSX/build/Debug/NewProject.app
(base) guycohen@Guys-MacBook-Air vst % 
```


to push release:

```sh
git tag v1.1.0 && git push origin v1.1.0
```