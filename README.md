# ct-bot
This repository contains the source code of the c't-Bot software framework. c't-Bot and c't-Sim belong together and represent a robot project that was initiated by the magazine [c't](https://www.heise.de/ct) in 2006.

The project website can be found at [www.ct-bot.de](https://www.ct-bot.de).
All related documentation is available at [here](https://github.com/Nightwalker-87/ct-bot-doku).

The repository here contains the source code of the original c't-Bot robot from 2006, referred to as *version 1* (v1). Each stable release of the code is tagged and the *master* branch always points to the latest one. The (experimental) development code can be found in branch *development*. Additional branches may exist for currently developed fixes or new features, use them on your own risk and bear in mind: if it breaks, you get to keep both pieces.

Feel free to fork from this repository, add your own extensions or improvements and create a pull request to get them integrated.

### Notes
Eclipse may update the file *.settings/language.settings.xml* every time your local build environment changes. To stop git complaining about these updates, close eclipse and execute the following command at the ct-bot top-level directory: <code>git update-index --skip-worktree -- .settings/language.settings.xml</code>.
You may need to repeat this for every branch you want to use.

### Continuous integration tests
| Branch              | Build status  |
|:------------------- |:------------- |
| master              | [![Build status](https://travis-ci.org/tsandmann/ct-bot.svg?branch=master "Build status of branch master")](https://travis-ci.org/tsandmann/ct-bot) |
| develop             | [![Build status](https://travis-ci.org/tsandmann/ct-bot.svg?branch=develop "Build status of branch develop")](https://travis-ci.org/tsandmann/ct-bot) |
