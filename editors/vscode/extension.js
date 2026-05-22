const vscode = require("vscode");
const { LanguageClient } = require("vscode-languageclient/node");

let client;

function activate(context) {
  const config = vscode.workspace.getConfiguration("walklang");
  const command = config.get("serverPath", "walk");

  client = new LanguageClient(
    "walklang",
    "WalkLang",
    {
      command,
      args: ["lsp"],
    },
    {
      documentSelector: [{ scheme: "file", language: "walk" }],
      synchronize: {
        fileEvents: vscode.workspace.createFileSystemWatcher("**/*.walk"),
      },
    }
  );

  client.start();
  context.subscriptions.push({
    dispose: () => {
      if (client) {
        client.stop();
      }
    },
  });
}

function deactivate() {
  if (!client) {
    return undefined;
  }
  return client.stop();
}

module.exports = {
  activate,
  deactivate,
};
