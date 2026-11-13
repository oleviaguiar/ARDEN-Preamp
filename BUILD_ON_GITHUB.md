# ARDEN — Compilar sem instalar programas no Windows

Este ZIP contém código-fonte e uma automação de compilação para GitHub Actions. **O VST3 não está compilado e o código ainda não foi validado por um build bem-sucedido.** Se a compilação falhar, será necessário corrigir o código e executar novamente.

1. Crie uma conta em https://github.com se ainda não tiver.
2. Crie um repositório novo, por exemplo `arden-preamp`, marcando-o como **Private**.
3. Extraia este ZIP e envie **o conteúdo da pasta ARDEN** para a raiz do repositório, inclusive a pasta oculta `.github` (se não aparecer, habilite arquivos ocultos no Explorador). No GitHub, use **Add file > Upload files** e selecione os arquivos e pastas; confira que `.github/workflows/build-windows.yml` foi incluído.
4. Abra a aba **Actions** do repositório. Se solicitado, habilite Actions. Selecione **Build ARDEN VST3 for Windows** e clique em **Run workflow**.
5. Aguarde o resultado. Se ficar verde, abra a execução e baixe o artefato **ARDEN-Windows-VST3**.
6. Extraia o ZIP baixado e copie a pasta `.vst3` para `C:\Program Files\Common Files\VST3`. Abra o Reaper/Ableton e execute uma nova varredura dos plugins.
7. Se ficar vermelho, abra a etapa que falhou, copie a mensagem de erro e envie aqui para correção.

Observação: GitHub Actions requer conexão com a internet para baixar o JUCE; a compilação pode demorar. O projeto v0.1 ainda não corresponde a todos os detalhes visuais e de DSP especificados e precisa de testes antes de uso em projetos importantes.
