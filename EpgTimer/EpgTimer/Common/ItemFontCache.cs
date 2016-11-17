using System.Collections.Generic;
using System.Windows;
using System.Windows.Media;

namespace EpgTimer
{
    static public class ItemFontCache
    {
        private static Dictionary<string, ItemFont> cache = new Dictionary<string, ItemFont>();
        static public void Clear() { cache.Clear(); }
        static public ItemFont ItemFont(string familyName, bool isBold)
        {
            string key = string.Format("{0}::{1}", familyName, isBold);
            if (cache.ContainsKey(key) == false)
            {
                cache.Add(key, new ItemFont(familyName, isBold));
            }
            return cache[key];
        }
    }
    public class ItemFont
    {
        public readonly GlyphTypeface GlyphType = null;
        private ushort[] indexCache = new ushort[ushort.MaxValue + 1];
        private float[] widthCache = new float[ushort.MaxValue + 1];
        public ItemFont(string familyName, bool isBold)
        {
            if ((new Typeface(new FontFamily(familyName),
                              FontStyles.Normal,
                              isBold ? FontWeights.Bold : FontWeights.Normal,
                              FontStretches.Normal)).TryGetGlyphTypeface(out GlyphType) == false)
            {
                (new Typeface(new FontFamily(System.Drawing.SystemFonts.DefaultFont.Name),
                              FontStyles.Normal,
                              isBold ? FontWeights.Bold : FontWeights.Normal,
                              FontStretches.Normal)).TryGetGlyphTypeface(out GlyphType);
            }
        }
        public ushort GlyphIndex(ushort code)
        {
            var glyphIndex = indexCache[code];
            if (glyphIndex == 0)
            {
                GlyphType.CharacterToGlyphMap.TryGetValue(code, out glyphIndex);
                indexCache[code] = glyphIndex;
                widthCache[glyphIndex] = (float)GlyphType.AdvanceWidths[glyphIndex];
            }
            return glyphIndex;
        }
        public double GlyphWidth(ushort glyphIndex)
        {
            return widthCache[glyphIndex];
        }
    }
}