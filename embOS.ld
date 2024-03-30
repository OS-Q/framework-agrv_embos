SECTIONS
{
  PROVIDE(__stack_start__ = __stack_pointer$ - __stack_size);
  PROVIDE(__stack_end__   = __stack_pointer$);
}
