# Comark

## Inline Components

Standalone: :icon-star :icon-heart

With content: :badge[New] :badge[v2.0]{color="blue"}

With props only: :tooltip{text="Hover me"}

## Block Components

::alert{type="info"}
This is an **info** alert with _inline_ formatting.
::

::card{title="Features"}

- Fast parsing
- Low memory
- Streaming output
  ::

Nested components:

:::layout{cols="2"}
::card{title="Left"}
First column content.
::

::card{title="Right"}
Second column content.
::
:::

## Inline Attributes

**bold**{.highlight} and _italic_{#myid} and `code`{.lang-ts}

~~deleted~~{.old} and **underline**{.accent}

[Link](https://github.com){target="\_blank" .external}

## Span Attributes

[Styled text]{.primary} and [**bold** inside span]{.fancy}

[Warning: check this]{.badge .rounded #alert-1}
