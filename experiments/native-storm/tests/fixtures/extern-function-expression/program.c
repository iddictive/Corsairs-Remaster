extern bool Custody_Begin(ref captor);

bool Local_Begin(ref captor)
{
    return true;
}

void Probe(ref NPChar)
{
    if (Custody_Begin(NPChar) == false)
    {
    }
    if (Local_Begin(NPChar) == false)
    {
    }
}
