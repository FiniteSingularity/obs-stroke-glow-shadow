<h1 align="center">Stroke, Glow, Shadow Plugin</h1>

<p align="center">
    <i align="center">An OBS plugin to create strokes, glow, and shadows around masked sources.</i>
</p>

<h2>Pre-Proudction. Currently only includes stroke.</h2>
<p>After installing, you'll find a "stroke" filter for your sources.  Select this filter, and play with the settings.  Enjoy.</p>
<p><i>Note- a much better readme is coming soon.</i></p>

### For Ubuntu

#### With GUI

- Download the Ubuntu .tar.gz for your Ubuntu version (currently 20.04 and 22.04 are supported)
- Extract the .tar.gz to a handy location.
- Using your file manager, navigate to `~/.config/obs-studio/plugins`
- Drop the `obs-stroke` folder into the plugins directory.
- (Re)start OBS and now you should be able to add the Stroke filter.

#### With Terminal

(Replace XX with either 20 or 22 for Ubuntus 20.04 & 22.04)
```bash
wget https://github.com/FiniteSingularity/obs-stroke-glow-shadow/releases/download/v1.0.2/obs-stroke-0.0.1a-ubuntu-XX.04.tar.gz.zip
unzip obs-stroke-0.0.1a-ubuntu-XX.04.tar.gz.zip
tar -zxf obs-stroke-(DATE)-(HASH)-ubuntu-XX.04.tar.gz
mkdir ~/.config/obs-studio/plugins
mv obs-stroke ~/.config/obs-studio/plugins
```
