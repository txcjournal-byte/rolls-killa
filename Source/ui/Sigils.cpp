#include "Sigils.h"

#include "../engine/CategoryProfile.h"

namespace rk::ui
{

void drawSigil (juce::Graphics& g, int category, juce::Rectangle<float> area, juce::Colour colour, float stroke)
{
    const auto r = area.reduced (area.getWidth() * 0.08f);
    const auto c = r.getCentre();
    const auto s = r.getWidth() * 0.5f;
    juce::Path p;

    auto poly = [&] (int sides, float radius, float rotation)
    {
        juce::Path q;
        q.addPolygon (c, sides, radius, rotation);
        return q;
    };

    switch (category)
    {
        case 0: // PRIMITIVUS - bone club: crossed bones
            p.startNewSubPath (c.x - s * 0.8f, c.y + s * 0.8f); p.lineTo (c.x + s * 0.8f, c.y - s * 0.8f);
            p.startNewSubPath (c.x - s * 0.8f, c.y - s * 0.8f); p.lineTo (c.x + s * 0.8f, c.y + s * 0.8f);
            for (auto [dx, dy] : { std::pair { -1.0f, -1.0f }, { 1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f } })
                p.addEllipse (c.x + dx * s * 0.8f - s * 0.16f, c.y + dy * s * 0.8f - s * 0.16f, s * 0.32f, s * 0.32f);
            break;

        case 1: // LIBER TRAP - triangle over an open book line
            p.addTriangle (c.x, c.y - s * 0.95f, c.x - s * 0.85f, c.y + s * 0.55f, c.x + s * 0.85f, c.y + s * 0.55f);
            p.startNewSubPath (c.x - s, c.y + s * 0.85f); p.lineTo (c.x, c.y + s * 0.7f); p.lineTo (c.x + s, c.y + s * 0.85f);
            p.addEllipse (c.x - s * 0.14f, c.y - s * 0.05f, s * 0.28f, s * 0.28f);
            break;

        case 2: // TRINITAS - three rings
            for (int i = 0; i < 3; ++i)
            {
                const auto pt = c.getPointOnCircumference (s * 0.42f, juce::MathConstants<float>::twoPi * (float) i / 3.0f);
                p.addEllipse (pt.x - s * 0.5f, pt.y - s * 0.5f, s, s);
            }
            break;

        case 3: // VOMITORIUM - spiral with a drip
        {
            const int turns = 60;
            for (int i = 0; i <= turns; ++i)
            {
                const auto a = (float) i / (float) turns * juce::MathConstants<float>::twoPi * 2.2f;
                const auto pt = c.getPointOnCircumference (s * 0.9f * (float) i / (float) turns, a);
                if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            p.startNewSubPath (c.x + s * 0.2f, c.y + s * 0.85f); p.lineTo (c.x + s * 0.2f, c.y + s * 1.1f);
            break;
        }

        case 4: // BLAST RITUAL - lightning through a circle
            p.addEllipse (c.x - s * 0.9f, c.y - s * 0.9f, s * 1.8f, s * 1.8f);
            p.startNewSubPath (c.x + s * 0.15f, c.y - s * 0.9f);
            p.lineTo (c.x - s * 0.3f, c.y + s * 0.05f);
            p.lineTo (c.x + s * 0.3f, c.y - s * 0.05f);
            p.lineTo (c.x - s * 0.15f, c.y + s * 0.9f);
            break;

        case 5: // AURA FARM - halo over a dot
            p.addCentredArc (c.x, c.y - s * 0.45f, s * 0.85f, s * 0.3f, 0.0f, 0.0f, juce::MathConstants<float>::twoPi, true);
            p.addEllipse (c.x - s * 0.35f, c.y + s * 0.05f, s * 0.7f, s * 0.7f);
            break;

        case 6: // DELIRIUM - melting eye
            p.startNewSubPath (c.x - s, c.y);
            p.quadraticTo (c.x, c.y - s * 0.8f, c.x + s, c.y);
            p.quadraticTo (c.x, c.y + s * 0.8f, c.x - s, c.y);
            p.addEllipse (c.x - s * 0.22f, c.y - s * 0.22f, s * 0.44f, s * 0.44f);
            p.startNewSubPath (c.x - s * 0.3f, c.y + s * 0.38f); p.lineTo (c.x - s * 0.3f, c.y + s * 0.95f);
            p.startNewSubPath (c.x + s * 0.2f, c.y + s * 0.4f); p.lineTo (c.x + s * 0.2f, c.y + s * 0.75f);
            break;

        case 7: // NECRODRILL - coffin with a drill line
            p.startNewSubPath (c.x - s * 0.35f, c.y - s);
            p.lineTo (c.x + s * 0.35f, c.y - s);
            p.lineTo (c.x + s * 0.6f, c.y - s * 0.45f);
            p.lineTo (c.x + s * 0.35f, c.y + s);
            p.lineTo (c.x - s * 0.35f, c.y + s);
            p.lineTo (c.x - s * 0.6f, c.y - s * 0.45f);
            p.closeSubPath();
            p.startNewSubPath (c.x, c.y - s * 0.6f); p.lineTo (c.x, c.y + s * 0.6f);
            p.startNewSubPath (c.x - s * 0.25f, c.y - s * 0.25f); p.lineTo (c.x + s * 0.25f, c.y - s * 0.25f);
            break;

        case 8: // MOTOR MORTIS - gear
        {
            p.addEllipse (c.x - s * 0.3f, c.y - s * 0.3f, s * 0.6f, s * 0.6f);
            const int teeth = 8;
            for (int i = 0; i < teeth; ++i)
            {
                const auto a0 = juce::MathConstants<float>::twoPi * (float) i / (float) teeth;
                const auto a1 = a0 + juce::MathConstants<float>::twoPi / (float) teeth * 0.5f;
                const auto pt0 = c.getPointOnCircumference (s * 0.65f, a0);
                const auto pt1 = c.getPointOnCircumference (s * 0.95f, a0);
                const auto pt2 = c.getPointOnCircumference (s * 0.95f, a1);
                const auto pt3 = c.getPointOnCircumference (s * 0.65f, a1);
                const auto pt4 = c.getPointOnCircumference (s * 0.65f, a0 + juce::MathConstants<float>::twoPi / (float) teeth);
                if (i == 0) p.startNewSubPath (pt0); else p.lineTo (pt0);
                p.lineTo (pt1); p.lineTo (pt2); p.lineTo (pt3); p.lineTo (pt4);
            }
            p.closeSubPath();
            break;
        }

        case 9: // SPASMUS - zigzag nerve
            p.startNewSubPath (c.x - s, c.y);
            p.lineTo (c.x - s * 0.6f, c.y);
            p.lineTo (c.x - s * 0.35f, c.y - s * 0.8f);
            p.lineTo (c.x, c.y + s * 0.8f);
            p.lineTo (c.x + s * 0.3f, c.y - s * 0.5f);
            p.lineTo (c.x + s * 0.55f, c.y + s * 0.3f);
            p.lineTo (c.x + s * 0.7f, c.y);
            p.lineTo (c.x + s, c.y);
            break;

        case 10: // REQUIEM - candle
            p.addRectangle (c.x - s * 0.25f, c.y - s * 0.2f, s * 0.5f, s * 1.1f);
            p.startNewSubPath (c.x, c.y - s * 0.95f);
            p.quadraticTo (c.x + s * 0.3f, c.y - s * 0.5f, c.x, c.y - s * 0.35f);
            p.quadraticTo (c.x - s * 0.3f, c.y - s * 0.5f, c.x, c.y - s * 0.95f);
            p.startNewSubPath (c.x - s * 0.7f, c.y + s * 0.9f); p.lineTo (c.x + s * 0.7f, c.y + s * 0.9f);
            break;

        case 11: // MANIA - broken spiral / crosshair
            p.addCentredArc (c.x, c.y, s * 0.85f, s * 0.85f, 0.0f, 0.3f, 2.8f, true);
            p.addCentredArc (c.x, c.y, s * 0.85f, s * 0.85f, 0.0f, 3.4f, 5.9f, true);
            p.addCentredArc (c.x, c.y, s * 0.45f, s * 0.45f, 0.0f, 1.8f, 5.2f, true);
            p.startNewSubPath (c.x, c.y - s); p.lineTo (c.x, c.y - s * 0.55f);
            p.startNewSubPath (c.x, c.y + s); p.lineTo (c.x, c.y + s * 0.55f);
            break;

        case 12: // USER - head + shoulders
            p.addEllipse (c.x - s * 0.32f, c.y - s * 0.9f, s * 0.64f, s * 0.64f);
            p.startNewSubPath (c.x - s * 0.8f, c.y + s * 0.9f);
            p.quadraticTo (c.x - s * 0.75f, c.y - s * 0.1f, c.x, c.y - s * 0.1f);
            p.quadraticTo (c.x + s * 0.75f, c.y - s * 0.1f, c.x + s * 0.8f, c.y + s * 0.9f);
            break;

        default: // FAVORITES - star (filled)
        {
            juce::Path star;
            star.addStar (c, 5, s * 0.42f, s, 0.0f);
            g.setColour (colour);
            g.fillPath (star);
            return;
        }
    }

    juce::ignoreUnused (poly);
    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (stroke, juce::PathStrokeType::mitered, juce::PathStrokeType::square));
}

//==============================================================================
namespace
{
    using Pts = std::initializer_list<juce::Point<float>>;

    void addPoly (juce::Path& path, Pts pts, float dx)
    {
        bool first = true;
        for (auto pt : pts)
        {
            if (first) path.startNewSubPath (pt.x + dx, pt.y);
            else       path.lineTo (pt.x + dx, pt.y);
            first = false;
        }
        path.closeSubPath();
    }

    constexpr float T = 3.6f;   // stroke thickness in glyph units (cap height 16)

    // Each glyph adds clockwise polygons (non-zero winding => union) and returns its advance.
    float glyphR (juce::Path& p, float x)
    {
        addPoly (p, { { 0, 0 }, { T, 0 }, { T, 16 }, { 1.8f, 18.5f }, { 0, 16 } }, x);                          // stem
        addPoly (p, { { 0, 0 }, { 9, 0 }, { 11, 2 }, { 11, 3.2f }, { 0, 3.2f } }, x);                          // top
        addPoly (p, { { 11 - T, 2 }, { 11, 2 }, { 11, 7 }, { 11 - T, 7 } }, x);                               // bowl side
        addPoly (p, { { 0, 5.8f }, { 11, 5.8f }, { 11, 7 }, { 9, 9 }, { 0, 9 } }, x);                          // middle
        addPoly (p, { { 5.2f, 8.2f }, { 9.0f, 8.2f }, { 11.2f, 16 }, { 10.4f, 19.5f }, { 7.4f, 16 } }, x);
        return 12.2f;
    }

    float glyphO (juce::Path& p, float x)
    {
        // ring drawn as 4 bars (keeps non-zero winding simple)
        addPoly (p, { { 2.5f, 0 }, { 8.5f, 0 }, { 11, 2.5f }, { 11, 3.4f }, { 0, 3.4f }, { 0, 2.5f } }, x);
        addPoly (p, { { 0, 12.6f }, { 11, 12.6f }, { 11, 13.5f }, { 8.5f, 16 }, { 2.5f, 16 }, { 0, 13.5f } }, x);
        addPoly (p, { { 0, 2.5f }, { T, 2.5f }, { T, 13.5f }, { 0, 13.5f } }, x);
        addPoly (p, { { 11 - T, 2.5f }, { 11, 2.5f }, { 11, 13.5f }, { 11 - T, 13.5f } }, x);
        return 12.2f;
    }

    float glyphL (juce::Path& p, float x)
    {
        addPoly (p, { { 0, 0 }, { T, 0 }, { T, 12.4f }, { 9.8f, 12.4f }, { 10.6f, 16 }, { 0, 16 } }, x);
        return 11.4f;
    }

    float glyphS (juce::Path& p, float x)
    {
        addPoly (p, { { 2.4f, 0 }, { 11, 0 }, { 11, 3.4f }, { T, 3.4f }, { T, 6.3f }, { 0, 6.3f }, { 0, 2.4f } }, x);
        addPoly (p, { { 0, 6.3f }, { 8.6f, 6.3f }, { 11, 8.7f }, { 11, 9.7f }, { 0, 9.7f } }, x);
        addPoly (p, { { 11 - T, 9.7f }, { 11, 9.7f }, { 11, 13.6f }, { 8.6f, 16 }, { 0, 16 }, { 0, 12.6f }, { 11 - T, 12.6f } }, x);
        return 12.2f;
    }

    float glyphK (juce::Path& p, float x)
    {
        addPoly (p, { { 0, 0 }, { T, 0 }, { T, 16 }, { 1.8f, 18.5f }, { 0, 16 } }, x);
        addPoly (p, { { 7.6f, 0 }, { 11.4f, 0 }, { 5.2f, 9.2f }, { T, 9.2f }, { T, 6.2f } }, x);
        addPoly (p, { { T, 6.6f }, { 6.4f, 6.6f }, { 11.4f, 16 }, { 10.8f, 19.8f }, { 7.4f, 16 }, { T, 10.2f } }, x);
        return 12.4f;
    }

    float glyphI (juce::Path& p, float x)
    {
        addPoly (p, { { 0, 0 }, { T, 0 }, { T, 16 }, { 1.8f, 19 }, { 0, 16 } }, x);
        return 4.8f;
    }

    float glyphA (juce::Path& p, float x)
    {
        addPoly (p, { { 4.4f, 0 }, { 6.0f, -2.6f }, { 7.6f, 0 }, { 12, 16 }, { 11.2f, 19.4f }, { 8.2f, 16 }, { 6.0f, 6.4f },
                      { 3.8f, 16 }, { 1.2f, 18.6f }, { 0, 16 } }, x);
        addPoly (p, { { 2.8f, 10.0f }, { 9.2f, 10.0f }, { 9.9f, 13.0f }, { 2.1f, 13.0f } }, x);
        return 13.2f;
    }
}

juce::Path createLogoPath()
{
    juce::Path p;
    p.setUsingNonZeroWinding (true);
    float x = 0.0f;

    x += glyphR (p, x); x += glyphO (p, x); x += glyphL (p, x); x += glyphL (p, x); x += glyphS (p, x);
    x += 5.0f;
    x += glyphK (p, x); x += glyphI (p, x); x += glyphL (p, x); x += glyphL (p, x); x += glyphA (p, x);

    // aggressive forward lean
    p.applyTransform (juce::AffineTransform::shear (-0.16f, 0.0f).translated (4.0f, 0.0f));
    return p;
}

juce::Image renderLogo (int height, float scale)
{
    auto path = createLogoPath();
    const auto bounds = path.getBounds();
    const auto pxHeight = (float) height * scale;
    const auto k = pxHeight * 0.78f / bounds.getHeight();
    const auto pad = pxHeight * 0.11f;
    const auto width = (int) std::ceil (bounds.getWidth() * k + pad * 2.0f);

    path.applyTransform (juce::AffineTransform::translation (-bounds.getX(), -bounds.getY()).scaled (k).translated (pad, pad));

    juce::Image glow (juce::Image::ARGB, width, (int) std::ceil (pxHeight), true);
    {
        juce::Graphics g (glow);
        for (int i = 6; i >= 1; --i)
        {
            g.setColour (colours::accent.withAlpha (0.045f));
            g.strokePath (path, juce::PathStrokeType ((float) i * pxHeight * 0.022f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const auto b = path.getBounds();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff5a66), b.getX(), b.getY(),
                                                 juce::Colour (0xffa80d1a), b.getX(), b.getBottom(), false));
        g.fillPath (path);

    }
    return glow;
}

} // namespace rk::ui
