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
        public string FamilyName { get; private set; }
        public bool IsBold { get; private set; }
        public GlyphTypeface GlyphType { get; private set; }
        public ushort[] GlyphIndexCache { get; private set; }
        public float[] GlyphWidthCache { get; private set; }

        public ItemFont(string familyName, bool isBold)
        {
            FamilyName = familyName;
            IsBold = isBold;
            GlyphTypeface glyphType = null;
            if ((new Typeface(new FontFamily(FamilyName),
                              FontStyles.Normal,
                              IsBold ? FontWeights.Bold : FontWeights.Normal,
                              FontStretches.Normal)).TryGetGlyphTypeface(out glyphType) == false)
            {
                (new Typeface(new FontFamily(System.Drawing.SystemFonts.DefaultFont.Name),
                              FontStyles.Normal,
                              IsBold ? FontWeights.Bold : FontWeights.Normal,
                              FontStretches.Normal)).TryGetGlyphTypeface(out glyphType);
            }
            GlyphType = glyphType;
            PrepareCache();
        }
        public void PrepareCache()
        {
            if (GlyphIndexCache == null)
            {
                GlyphIndexCache = new ushort[ushort.MaxValue + 1];
                GlyphWidthCache = new float[ushort.MaxValue + 1];
            }
        }
        public void ClearCache()
        {
            GlyphIndexCache = null;
            GlyphWidthCache = null;
        }
    }
}