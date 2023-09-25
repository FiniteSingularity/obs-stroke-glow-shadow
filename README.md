<h1 align="center">Stroke, Glow, Shadow Plugin LOGO</h1>

<p align="center">
    <i align="center">An OBS plugin to create strokes, glow, and shadows around masked sources.</i>
</p>

<h4 align="center">
    <a href="https://github.com/FiniteSingularity/obs-stroke-glow-shadow/releases">
        <img src="https://img.shields.io/github/v/release/finitesingularity/obs-stroke-glow-shadow?filter=*&style=flat-square&label=Latest" alt="Latest release">
    </a>
    <img src="https://img.shields.io/badge/OBS-28_|_29_|_30-blue.svg?style=flat-square" alt="supports obs versions 28, 29, and 30">
    <img src="https://img.shields.io/badge/Windows-0078D6?style=flat-square&logo=windows&logoColor=white">
    <img src="https://img.shields.io/badge/mac%20os-000000?style=flat-square&logo=apple&logoColor=white">
    <img src="https://img.shields.io/badge/Linux-FCC624?style=flat-square&logo=linux&logoColor=black"><br>
    <a href="https://twitter.com/FiniteSingulrty">
        <img src="https://img.shields.io/badge/Twitter-1DA1F2?style=flat-square&logo=twitter&logoColor=white">
    </a>
    <a href="https://twitch.tv/finitesingularity">
        <img src="https://img.shields.io/badge/Twitch-9146FF?style=flat-square&logo=twitch&logoColor=white">
    </a>
</h4>

> **Note**
> While we only release new versions of the Stroke, Glow, Shadow plugin after testing it on local hardware, there are bugs and issues that will slip through. If you happen to run into any issues, please [open an issue](https://github.com/finitesingularity/obs-stroke-glow-shadow/issues) and we will work to resolve it.

## Introduction

The Stroke, Glow, Shadow Plugin allows for efficient Stroke, Glow, and Shadow effects on masked sources.

- 🚀 Stroke, Glow, Shadow, as the name implies, provides effects for applying [Stroke](#stroke), [Glow](#glow), and [Drop Shadow](#shadow) to any masked sources in OBS. The effects can be applied to everything from a chroma-keyed facecam to a native OBS text source.
- ↔️ All three effects allow for inner and outer applications.
- 🎨 All three effects also allow for a [solid color](#color-fill) or [another OBS source](#source-fill) to be used for the fill.
- 🎛️ Finally, all three effects are provided as both [inline filters](#filter) and as [separate sources](#source).

## Effect Types

### Stroke

The stroke effect draws a line with a user-definable stroke size and offset around the masked source. If offset is being used, there is an option to draw the stroke with a more efficient approximate method, or a more GPU intensive high accuracy method. If the stroke effect is being used as a source it requires the user to provide a separate OBS source that will be used to generate the stroke. The source version of the effect does not redraw the selcted source, however it does give the option of in-filling the stroke in cases where there is no offset.

### Glow

The glow effect draws a feathered stroke with a user-defined size and intensity around the masked source.

### Shadow

The shadow effect draws an offset drop-shadow simulating a light causing the selected source to cast a shadow, and giving the effect of the source floating off the canvas.
