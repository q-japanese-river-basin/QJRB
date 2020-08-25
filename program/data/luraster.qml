<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis version="3.4.9-Madeira" maxScale="0" minScale="1e+08" hasScaleBasedVisibilityFlag="0" styleCategories="AllStyleCategories">
  <flags>
    <Identifiable>1</Identifiable>
    <Removable>1</Removable>
    <Searchable>1</Searchable>
  </flags>
  <customproperties>
    <property value="false" key="WMSBackgroundLayer"/>
    <property value="false" key="WMSPublishDataSourceUrl"/>
    <property value="0" key="embeddedWidgets/count"/>
    <property value="Value" key="identify/format"/>
  </customproperties>
  <pipe>
    <rasterrenderer type="paletted" opacity="1" alphaBand="-1" band="1">
      <rasterTransparency/>
      <minMaxOrigin>
        <limits>None</limits>
        <extent>WholeRaster</extent>
        <statAccuracy>Estimated</statAccuracy>
        <cumulativeCutLower>0.02</cumulativeCutLower>
        <cumulativeCutUpper>0.98</cumulativeCutUpper>
        <stdDevFactor>2</stdDevFactor>
      </minMaxOrigin>
      <colorPalette>
        <paletteEntry color="#fefefe" alpha="255" value="0" label="解析範囲外"/>
        <paletteEntry color="#fefe00" alpha="255" value="10" label="田"/>
        <paletteEntry color="#fecb98" alpha="255" value="20" label="その他農用地"/>
        <paletteEntry color="#00a900" alpha="255" value="50" label="森林"/>
        <paletteEntry color="#fe9800" alpha="255" value="60" label="荒地"/>
        <paletteEntry color="#fe0000" alpha="255" value="70" label="建物用地"/>
        <paletteEntry color="#8b8b8b" alpha="255" value="91" label="道路"/>
        <paletteEntry color="#b3b3b3" alpha="255" value="92" label="鉄道"/>
        <paletteEntry color="#c7450e" alpha="255" value="100" label="その他の用地"/>
        <paletteEntry color="#0000fe" alpha="255" value="110" label="河川地及び湖沼"/>
        <paletteEntry color="#fefe98" alpha="255" value="140" label="海浜"/>
        <paletteEntry color="#00cbfe" alpha="255" value="150" label="海水域"/>
        <paletteEntry color="#00fe00" alpha="255" value="160" label="ゴルフ場"/>
        <paletteEntry color="#fefefe" alpha="255" value="255" label="解析範囲外"/>
      </colorPalette>
      <colorramp type="randomcolors" name="[source]"/>
    </rasterrenderer>
    <brightnesscontrast brightness="0" contrast="0"/>
    <huesaturation colorizeBlue="128" saturation="0" colorizeOn="0" colorizeStrength="100" grayscaleMode="0" colorizeRed="255" colorizeGreen="128"/>
    <rasterresampler maxOversampling="2"/>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
