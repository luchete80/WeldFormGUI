# core/BaseFEMScene.py
from pathlib import Path
from manim import *
import os

REPO_ROOT = Path(__file__).resolve().parents[1]
ASSETS_DIR = REPO_ROOT / "assets"
INTRO_FILE = ASSETS_DIR / "intro.mp4"
LOGO_FILE = ASSETS_DIR / "logo.png"

teal     = "#10B981"
purple   = "#9d4edd"
navy     = "#2c4d85"
charcoal = "#111827"
amber    = "#f59e0b"
white    = "#f2f4f6"

# Visual constants (brand)
COLORS = {
    "bg": charcoal,
    "title": navy,
    "text": "#F2F4F6",
    "accent": "#00BFA5",   # teal
    "accent2": "#7e57c2",  # purple
    "accent3": "#ffb300",  # amber
}

__all__ = [
    "BaseFEMScene",
    "COLORS",
    "teal",
    "purple",
    "navy",
    "charcoal",
    "amber",
    "white"
]

FIGTREE_FONT_NAME = "FigTree"  # ensure FigTree.ttf exists in assets/fonts or system

class BaseFEMScene(Scene):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.camera.background_color = COLORS["bg"]

    def add_labeled_equation(
            self,
            label_text,
            equation_tex,
            color,
            anchor=None,
            pre_position=None,
            vertical_spacing=1.2,
            equation_offset=0.15,
            size = 32,
            equation_side="down"   # <-- NUEVO
        ):

        # Convertimos la dirección de texto a un vector de Manim
        side_map = {
            "down": DOWN,
            "up": UP,
            "left": LEFT,
            "right": RIGHT
        }
        direction = side_map.get(equation_side, DOWN)

        label = Text(
            label_text,
            font_size=size,
            color=color,
            stroke_width=3,
            stroke_color=color,
            font=FIGTREE_FONT_NAME
        )

        equation = MathTex(
            equation_tex,
            color=color
        ).next_to(label, direction, buff=equation_offset)

        group = VGroup(label, equation)

        if anchor is not None:
            group.move_to(anchor.get_center() + DOWN * vertical_spacing)

        if pre_position is not None:
            group.move_to(pre_position)

        self.play(Write(label))
        self.play(Write(equation))

        return label, equation




    def add_title_text(self, text, duration=1.2):
        """Write a title centered using FigTree if available."""
        try:
            title = Text(text, font=FIGTREE_FONT_NAME, weight=BOLD, font_size=48, color=COLORS["text"])
        except Exception:
            title = Text(text, font_size=48, color=COLORS["text"])
        self.play(Write(title), run_time=duration)
        return title

    def add_title(self, text, font_size=42, color=None, stroke_color=None, stroke_width=4, animate=True):
        """
        Agrega un título estilizado en la parte superior.
        Devuelve el objeto Text para manipularlo luego.
        """
        color = color or COLORS["title"]
        stroke_color = stroke_color or color

        title = Text(
            text,
            font=FIGTREE_FONT_NAME,
            color=color,
            font_size=font_size,
            stroke_width=stroke_width,
            stroke_color=stroke_color
        ).to_edge(UP)

        if animate:
            self.play(Write(title))
        else:
            self.add(title)
        return title

    def add_text(
            self,
            text,
            font_size=28,
            color=None,
            position=DOWN,
            shift=0.4,
            animate=True,
            duration=0.6,
            stroke_width=3,
        ):
        """
        Agrega texto narrado/explicativo.
        - color: usa COLORS["text"] por default.
        - position: UP, DOWN, LEFT, RIGHT, ORIGIN, etc.
        - shift: cuánto desplazar desde position.
        - animate: Write() si True, Add() si False.
        """

        color = color or COLORS["text"]

        # Construcción con FigTree si se puede
        try:
            obj = Text(text, font=FIGTREE_FONT_NAME, font_size=font_size, stroke_width=stroke_width, stroke_color=color  ,color=color)
        except Exception:
            obj = Text(text, font_size=font_size, stroke_width=stroke_width, stroke_color=color  , color=color)

        # posicionamiento global tipo subtítulo
        obj.to_edge(position).shift(shift * position)

        # animación
        if animate:
            self.play(Write(obj), run_time=duration)
        #else:
            #self.add(obj)

        return obj


    def add_math_text(
            self,
            text,
            font_size=28,
            color=None,
            position=DOWN,
            shift=0.4,
            animate=False,
            duration=0.6,
            stroke_width=4,
        ):
        """
        Agrega texto narrado/explicativo.
        - color: usa COLORS["text"] por default.
        - position: UP, DOWN, LEFT, RIGHT, ORIGIN, etc.
        - shift: cuánto desplazar desde position.
        - animate: Write() si True, Add() si False.
        """

        color = color or COLORS["text"]

        # Construcción con FigTree si se puede
        try:
            obj = MathTex(text, font=FIGTREE_FONT_NAME, font_size=font_size, stroke_width=stroke_width, color=color)
        except Exception:
            obj = MathTex(text, font_size=font_size, stroke_width=stroke_width, color=color)

        # posicionamiento global tipo subtítulo
        obj.to_edge(position).shift(shift * position)

        # animación
        if animate:
            self.play(Write(obj), run_time=duration)
        else:
            self.add(obj)

        return obj
        
    def show_hook(self, hook_lines, hold=10.0):
        """
        Display a hook (first ~10s). hook_lines can be list[str] or single str.
        This method places the text, can animate it. Total hold ~= hold seconds.
        """
        if isinstance(hook_lines, str):
            hook_lines = [hook_lines]
        group = VGroup()
        for i, line in enumerate(hook_lines):
            try:
                t = Text(line, font=FIGTREE_FONT_NAME, weight=BOLD, font_size=32, color=COLORS["text"])
            except Exception:
                t = Text(line, font_size=32, color=COLORS["text"])
            t.to_edge(UP).shift(DOWN * (i * 0.9))
            group.add(t)
        # show lines sequentially
        for t in group:
            self.play(FadeIn(t, shift=UP * 0.2), run_time=0.6)
            self.wait(0.2)
        # hold
        self.wait(max(0.5, hold - 0.8 * len(group)))
        return group

    def play_intro_video(self, intro_duration=5.0):
        """
        Try to play intro.mp4 if manim supports VideoFileClip in your version.
        If not available, display a placeholder image (first frame) or skip.
        Returns actual duration used.
        """
        intro_path = INTRO_FILE
        if intro_path.exists():
            try:
                # Movie/Video support varies across Manim versions.
                # Use VideoFileClip if available, else fallback to ImageMobject of a poster.
                try:
                    movie = VideoFileClip(str(intro_path))
                    movie = movie.set_width(FRAME_WIDTH)
                    mobj = ImageMobject(intro_path)  # fallback also used if VideoFileClip fails
                    # many manim installs accept ImageMobject but not video playback in Scene
                    self.play(FadeIn(mobj), run_time=0.6)
                    self.wait(intro_duration)
                    self.play(FadeOut(mobj), run_time=0.4)
                    return intro_duration
                except Exception:
                    # Fallback: show a static poster (logo on top of a rectangle)
                    poster = ImageMobject(intro_path)
                    poster.scale_to_fit_width(FRAME_WIDTH)
                    self.play(FadeIn(poster), run_time=0.6)
                    self.wait(intro_duration)
                    self.play(FadeOut(poster), run_time=0.4)
                    return intro_duration
            except Exception:
                # final fallback: blank hold
                self.wait(intro_duration)
                return intro_duration
        else:
            # no intro file: just wait
            self.wait(intro_duration)
            return intro_duration

    def add_logo_corner(self, scale=0.12, buff=0.3):
        """
        Put a static logo PNG in the bottom-right corner, scaled and with slight opacity.
        """
        logo_path = LOGO_FILE
        if logo_path.exists():
            try:
                img = ImageMobject(str(logo_path))
                img.scale(scale)
                # position bottom-right
                img.to_corner(DR, buff=buff)
                img.set_opacity(0.95)
                self.add(img)
                return img
            except Exception:
                return None
        else:
            return None

    # small helper to ensure font file is loadable if you placed it inside assets/fonts
    @staticmethod
    def ensure_figtree_loaded():
        fonts_dir = ASSETS_DIR / "fonts"
        if fonts_dir.exists():
            for f in fonts_dir.glob("FigTree*"):
                # some systems auto-detect font files in local folder; manim uses system fonts
                os.environ["MPLCONFIGDIR"] = str(fonts_dir)
                break

